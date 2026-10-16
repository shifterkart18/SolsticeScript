#include "SCodeGenerator.h"

#include <array>
#include <iostream>
#include <utility>

namespace Solstice
{
	namespace
	{
		const char* OpCodeName(OpCode op)
		{
			constexpr std::array<const char*, 33> Names =
			{
				"ADD", "SUBTRACT", "MULTIPLY", "DIVIDE", "EQUAL", "NOT_EQUAL",
				"LESS", "GREATER", "GREATER_EQUAL", "LESS_EQUAL", "NOT", "NEGATE",
				"CONCAT", "TO_BOOLEAN", "PUSH_NIL", "PUSH_BOOLEAN", "PUSH_NUMBER",
				"PUSH_STRING", "MAKE_ARRAY", "LOAD_LOCAL", "STORE_LOCAL",
				"LOAD_GLOBAL", "STORE_GLOBAL", "GET_INDEX", "SET_INDEX", "POP",
				"JUMP", "JUMP_IF_FALSE", "JUMP_IF_TRUE", "CALL", "NATIVE_CALL",
				"RETURN", "HALT"
			};
			return Names.at(static_cast<std::size_t>(op));
		}
	}

	SCodeGenerator::SCodeGenerator(
		std::shared_ptr<const std::vector<NativeFunctionDefinition>> nativeFunctions)
		: mNativeFunctions(std::move(nativeFunctions))
	{
	}

	bool SCodeGenerator::Build(const std::string& fileName, const CompileOptions& options)
	{
		mProgram = {};
		mFunctions.clear();
		mCurrentFunction.clear();
		mParser = std::make_unique<SParser>(*mNativeFunctions);

		if(options.dumpTokens)
		{
			SLexicalAnalyzer lexer(fileName);
			lexer.DisplayTokens();
		}

		mRootNode = mParser->Parse(fileName);
		mFunctions = mParser->Functions();
		mProgram.globalCount = mParser->GlobalCount();

		if(options.dumpAst)
			mRootNode->PrintTree(std::cout);

		for(const NodePtr& child : mRootNode->children)
		{
			if(child->type == NT_FUNCTION)
				GenerateFunction(*child);
		}

		mProgram.entryPoint = mProgram.instructions.size();
		for(const NodePtr& child : mRootNode->children)
		{
			if(child->type != NT_FUNCTION)
				GenerateNode(*child);
		}

		Emit(OpCode::Halt, mRootNode->location);
		ResolveCalls();

		if(options.dumpBytecode)
			PrintDebugInfo();

		return true;
	}

	const CompiledProgram& SCodeGenerator::Program() const noexcept
	{
		return mProgram;
	}

	void SCodeGenerator::PrintDebugInfo() const
	{
		for(std::size_t index = 0; index < mProgram.instructions.size(); ++index)
		{
			const Instruction& instruction = mProgram.instructions[index];
			std::cout << index << "\t" << OpCodeName(instruction.op);

			switch(instruction.op)
			{
			case OpCode::PushNumber:
				std::cout << "\t" << instruction.number;
				break;
			case OpCode::PushBoolean:
				std::cout << "\t" << (instruction.operand != 0 ? "true" : "false");
				break;
			case OpCode::PushString:
			case OpCode::Call:
				std::cout << "\t" << instruction.text;
				break;
			case OpCode::LoadLocal:
			case OpCode::StoreLocal:
			case OpCode::LoadGlobal:
			case OpCode::StoreGlobal:
			case OpCode::MakeArray:
			case OpCode::Jump:
			case OpCode::JumpIfFalse:
			case OpCode::JumpIfTrue:
				std::cout << "\t" << instruction.operand;
				break;
			case OpCode::NativeCall:
				std::cout << "\t" << instruction.operand << ", " << instruction.operand2;
				break;
			default:
				break;
			}

			std::cout << '\n';
		}
	}

	void SCodeGenerator::GenerateNode(const SNode& node)
	{
		switch(node.type)
		{
		case NT_PROGRAM:
		case NT_FUNCTION:
			return;
		case NT_BLOCKSTATEMENT:
			for(const NodePtr& child : node.children)
				GenerateNode(*child);
			return;
		case NT_VOID:
			GenerateNode(*node.children[0]);
			Emit(OpCode::Pop, node.location);
			return;
		case NT_LOCALASSIGN:
			GenerateNode(*node.children[1]);
			GenerateStore(
				static_cast<std::size_t>(node.children[0]->slot),
				node.children[0]->type == NT_GLOBALIDENT,
				node.location);
			Emit(OpCode::Pop, node.location);
			return;
		case NT_ASSIGN:
			GenerateAssignment(node);
			return;
		case NT_IF:
			GenerateIf(node);
			return;
		case NT_WHILE:
			GenerateWhile(node);
			return;
		case NT_FOR:
			GenerateFor(node);
			return;
		case NT_REPEAT:
			GenerateRepeat(node);
			return;
		case NT_DO:
			GenerateNode(*node.children[0]);
			return;
		case NT_RETURN:
			if(node.children.empty())
				Emit(OpCode::PushNil, node.location);
			else
				GenerateNode(*node.children[0]);
			Emit(OpCode::Return, node.location);
			return;
		case NT_AND:
			GenerateAnd(node);
			return;
		case NT_OR:
			GenerateOr(node);
			return;
		case NT_ARRAY:
			for(const NodePtr& child : node.children)
				GenerateNode(*child);
			{
				std::size_t instruction = Emit(OpCode::MakeArray, node.location);
				mProgram.instructions[instruction].operand = node.children.size();
			}
			return;
		case NT_INDEX:
			GenerateNode(*node.children[0]);
			GenerateNode(*node.children[1]);
			Emit(OpCode::GetIndex, node.location);
			return;
		case NT_FUNCTIONCALL:
			for(const NodePtr& argument : node.children)
				GenerateNode(*argument);
			{
				std::size_t instruction = Emit(OpCode::Call, node.location);
				mProgram.instructions[instruction].text = node.text;
				mProgram.instructions[instruction].operand2 = node.children.size();
			}
			return;
		case NT_REGFUNCCALL:
			for(const NodePtr& argument : node.children)
				GenerateNode(*argument);
			{
				const NativeFuncInfo& function = mParser->NativeFunctions().at(node.text);
				std::size_t instruction = Emit(OpCode::NativeCall, node.location);
				mProgram.instructions[instruction].operand = function.index;
				mProgram.instructions[instruction].operand2 = node.children.size();
			}
			return;
		case NT_IDENT:
			GenerateLoad(static_cast<std::size_t>(node.slot), false, node.location);
			return;
		case NT_GLOBALIDENT:
			GenerateLoad(static_cast<std::size_t>(node.slot), true, node.location);
			return;
		case NT_NIL:
			Emit(OpCode::PushNil, node.location);
			return;
		case NT_BOOLEAN:
			{
				std::size_t instruction = Emit(OpCode::PushBoolean, node.location);
				mProgram.instructions[instruction].operand = node.boolValue ? 1 : 0;
			}
			return;
		case NT_NUMBER:
			{
				std::size_t instruction = Emit(OpCode::PushNumber, node.location);
				mProgram.instructions[instruction].number = node.numberValue;
			}
			return;
		case NT_STRING:
			{
				std::size_t instruction = Emit(OpCode::PushString, node.location);
				mProgram.instructions[instruction].text = node.text;
			}
			return;
		default:
			break;
		}

		for(const NodePtr& child : node.children)
			GenerateNode(*child);

		OpCode op = OpCode::Halt;
		switch(node.type)
		{
		case NT_ADD:
			op = OpCode::Add;
			break;
		case NT_SUB:
			op = OpCode::Subtract;
			break;
		case NT_MULT:
			op = OpCode::Multiply;
			break;
		case NT_DIV:
			op = OpCode::Divide;
			break;
		case NT_EQUALS:
			op = OpCode::Equal;
			break;
		case NT_NOTEQUALS:
			op = OpCode::NotEqual;
			break;
		case NT_LESS:
			op = OpCode::Less;
			break;
		case NT_GREATER:
			op = OpCode::Greater;
			break;
		case NT_GREATEQUAL:
			op = OpCode::GreaterEqual;
			break;
		case NT_LESSEQUAL:
			op = OpCode::LessEqual;
			break;
		case NT_NOT:
			op = OpCode::Not;
			break;
		case NT_NEG:
			op = OpCode::Negate;
			break;
		case NT_CONCAT:
			op = OpCode::Concat;
			break;
		default:
			throw SException(node.location, "Unsupported syntax node");
		}

		Emit(op, node.location);
	}

	void SCodeGenerator::GenerateFunction(const SNode& node)
	{
		mCurrentFunction = node.text;
		FuncDefInfo& information = mFunctions.at(node.text);
		information.offset = mProgram.instructions.size();
		GenerateNode(*node.children[0]);
		Emit(OpCode::PushNil, node.location);
		Emit(OpCode::Return, node.location);
	}

	void SCodeGenerator::GenerateAssignment(const SNode& node)
	{
		const SNode& target = *node.children[0];
		if(target.type == NT_INDEX)
		{
			GenerateNode(*target.children[0]);
			GenerateNode(*target.children[1]);
			GenerateNode(*node.children[1]);
			Emit(OpCode::SetIndex, node.location);
			return;
		}

		GenerateNode(*node.children[1]);
		GenerateStore(
			static_cast<std::size_t>(target.slot),
			target.type == NT_GLOBALIDENT,
			node.location);
	}

	void SCodeGenerator::GenerateIf(const SNode& node)
	{
		GenerateNode(*node.children[0]);
		std::size_t falseJump = Emit(OpCode::JumpIfFalse, node.location);
		GenerateNode(*node.children[1]);

		if(node.children.size() == 2)
		{
			PatchJump(falseJump, mProgram.instructions.size());
			return;
		}

		std::size_t endJump = Emit(OpCode::Jump, node.location);
		PatchJump(falseJump, mProgram.instructions.size());
		GenerateNode(*node.children[2]);
		PatchJump(endJump, mProgram.instructions.size());
	}

	void SCodeGenerator::GenerateWhile(const SNode& node)
	{
		std::size_t loopStart = mProgram.instructions.size();
		GenerateNode(*node.children[0]);
		std::size_t exitJump = Emit(OpCode::JumpIfFalse, node.location);
		GenerateNode(*node.children[1]);
		std::size_t jump = Emit(OpCode::Jump, node.location);
		mProgram.instructions[jump].operand = loopStart;
		PatchJump(exitJump, mProgram.instructions.size());
	}

	void SCodeGenerator::GenerateFor(const SNode& node)
	{
		std::size_t slot = static_cast<std::size_t>(node.slot);
		std::size_t endSlot = static_cast<std::size_t>(node.auxiliarySlot);
		std::size_t stepSlot = static_cast<std::size_t>(node.auxiliarySlot2);

		GenerateNode(*node.children[0]);
		GenerateStore(slot, node.global, node.location);
		Emit(OpCode::Pop, node.location);
		GenerateNode(*node.children[1]);
		GenerateStore(endSlot, node.global, node.location);
		Emit(OpCode::Pop, node.location);
		GenerateNode(*node.children[2]);
		GenerateStore(stepSlot, node.global, node.location);
		Emit(OpCode::Pop, node.location);

		std::size_t loopStart = mProgram.instructions.size();
		GenerateLoad(stepSlot, node.global, node.location);
		{
			std::size_t zero = Emit(OpCode::PushNumber, node.location);
			mProgram.instructions[zero].number = 0.0;
		}
		Emit(OpCode::GreaterEqual, node.location);
		std::size_t negativeStep = Emit(OpCode::JumpIfFalse, node.location);

		GenerateLoad(slot, node.global, node.location);
		GenerateLoad(endSlot, node.global, node.location);
		Emit(OpCode::LessEqual, node.location);
		std::size_t positiveExit = Emit(OpCode::JumpIfFalse, node.location);
		std::size_t bodyJump = Emit(OpCode::Jump, node.location);

		PatchJump(negativeStep, mProgram.instructions.size());
		GenerateLoad(slot, node.global, node.location);
		GenerateLoad(endSlot, node.global, node.location);
		Emit(OpCode::GreaterEqual, node.location);
		std::size_t negativeExit = Emit(OpCode::JumpIfFalse, node.location);

		PatchJump(bodyJump, mProgram.instructions.size());
		GenerateNode(*node.children[3]);
		GenerateLoad(slot, node.global, node.location);
		GenerateLoad(stepSlot, node.global, node.location);
		Emit(OpCode::Add, node.location);
		GenerateStore(slot, node.global, node.location);
		Emit(OpCode::Pop, node.location);

		std::size_t loopJump = Emit(OpCode::Jump, node.location);
		mProgram.instructions[loopJump].operand = loopStart;
		std::size_t loopEnd = mProgram.instructions.size();
		PatchJump(positiveExit, loopEnd);
		PatchJump(negativeExit, loopEnd);
	}

	void SCodeGenerator::GenerateRepeat(const SNode& node)
	{
		std::size_t loopStart = mProgram.instructions.size();
		GenerateNode(*node.children[0]);
		GenerateNode(*node.children[1]);
		std::size_t jump = Emit(OpCode::JumpIfFalse, node.location);
		mProgram.instructions[jump].operand = loopStart;
	}

	void SCodeGenerator::GenerateAnd(const SNode& node)
	{
		GenerateNode(*node.children[0]);
		std::size_t falseJump = Emit(OpCode::JumpIfFalse, node.location);
		GenerateNode(*node.children[1]);
		Emit(OpCode::ToBoolean, node.location);
		std::size_t endJump = Emit(OpCode::Jump, node.location);
		PatchJump(falseJump, mProgram.instructions.size());
		{
			std::size_t instruction = Emit(OpCode::PushBoolean, node.location);
			mProgram.instructions[instruction].operand = 0;
		}
		PatchJump(endJump, mProgram.instructions.size());
	}

	void SCodeGenerator::GenerateOr(const SNode& node)
	{
		GenerateNode(*node.children[0]);
		std::size_t trueJump = Emit(OpCode::JumpIfTrue, node.location);
		GenerateNode(*node.children[1]);
		Emit(OpCode::ToBoolean, node.location);
		std::size_t endJump = Emit(OpCode::Jump, node.location);
		PatchJump(trueJump, mProgram.instructions.size());
		{
			std::size_t instruction = Emit(OpCode::PushBoolean, node.location);
			mProgram.instructions[instruction].operand = 1;
		}
		PatchJump(endJump, mProgram.instructions.size());
	}

	void SCodeGenerator::GenerateLoad(
		std::size_t slot,
		bool global,
		const SourceLocation& location)
	{
		std::size_t instruction = Emit(
			global ? OpCode::LoadGlobal : OpCode::LoadLocal,
			location);
		mProgram.instructions[instruction].operand = slot;
	}

	void SCodeGenerator::GenerateStore(
		std::size_t slot,
		bool global,
		const SourceLocation& location)
	{
		std::size_t instruction = Emit(
			global ? OpCode::StoreGlobal : OpCode::StoreLocal,
			location);
		mProgram.instructions[instruction].operand = slot;
	}

	std::size_t SCodeGenerator::Emit(OpCode op, const SourceLocation& location)
	{
		mProgram.instructions.push_back(Instruction{op, location});
		return mProgram.instructions.size() - 1;
	}

	void SCodeGenerator::PatchJump(std::size_t instruction, std::size_t target)
	{
		mProgram.instructions.at(instruction).operand = target;
	}

	void SCodeGenerator::ResolveCalls()
	{
		for(Instruction& instruction : mProgram.instructions)
		{
			if(instruction.op != OpCode::Call)
				continue;

			auto function = mFunctions.find(instruction.text);
			if(function == mFunctions.end())
			{
				throw SException(
					instruction.location,
					"Attempting to call undeclared function '" + instruction.text + "'");
			}

			if(instruction.operand2 != function->second.numArgs)
			{
				throw SException(
					instruction.location,
					"Function '" + instruction.text + "' expects " +
						std::to_string(function->second.numArgs) + " argument(s)");
			}

			instruction.operand = function->second.offset;
			instruction.operand3 = function->second.numLocalVars;
		}
	}
}
