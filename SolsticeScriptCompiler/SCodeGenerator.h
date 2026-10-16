#pragma once

#include "SFunctions.h"
#include "SParser.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Solstice
{
	enum class OpCode
	{
		Add,
		Subtract,
		Multiply,
		Divide,
		Equal,
		NotEqual,
		Less,
		Greater,
		GreaterEqual,
		LessEqual,
		Not,
		Negate,
		Concat,
		ToBoolean,
		PushNil,
		PushBoolean,
		PushNumber,
		PushString,
		MakeArray,
		LoadLocal,
		StoreLocal,
		LoadGlobal,
		StoreGlobal,
		GetIndex,
		SetIndex,
		Pop,
		Jump,
		JumpIfFalse,
		JumpIfTrue,
		Call,
		NativeCall,
		Return,
		Halt
	};

	struct Instruction
	{
		OpCode op = OpCode::Halt;
		SourceLocation location;
		double number = 0.0;
		std::size_t operand = 0;
		std::size_t operand2 = 0;
		std::size_t operand3 = 0;
		std::string text;
	};

	struct CompiledProgram
	{
		std::vector<Instruction> instructions;
		std::size_t entryPoint = 0;
		std::size_t globalCount = 0;
	};

	struct CompileOptions
	{
		bool dumpTokens = false;
		bool dumpAst = false;
		bool dumpBytecode = false;
	};

	class SCodeGenerator
	{
	public:
		explicit SCodeGenerator(
			std::shared_ptr<const std::vector<NativeFunctionDefinition>> nativeFunctions);

		bool Build(const std::string& fileName, const CompileOptions& options = {});
		const CompiledProgram& Program() const noexcept;
		void PrintDebugInfo() const;

	private:
		void GenerateNode(const SNode& node);
		void GenerateFunction(const SNode& node);
		void GenerateAssignment(const SNode& node);
		void GenerateIf(const SNode& node);
		void GenerateWhile(const SNode& node);
		void GenerateFor(const SNode& node);
		void GenerateRepeat(const SNode& node);
		void GenerateAnd(const SNode& node);
		void GenerateOr(const SNode& node);
		void GenerateLoad(std::size_t slot, bool global, const SourceLocation& location);
		void GenerateStore(std::size_t slot, bool global, const SourceLocation& location);
		std::size_t Emit(OpCode op, const SourceLocation& location);
		void PatchJump(std::size_t instruction, std::size_t target);
		void ResolveCalls();

		std::shared_ptr<const std::vector<NativeFunctionDefinition>> mNativeFunctions;
		std::unique_ptr<SParser> mParser;
		NodePtr mRootNode;
		std::unordered_map<std::string, FuncDefInfo> mFunctions;
		CompiledProgram mProgram;
		std::string mCurrentFunction;
	};
}