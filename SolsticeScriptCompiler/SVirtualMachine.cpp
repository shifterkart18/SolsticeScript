#include "SVirtualMachine.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace Solstice
{
	SVirtualMachine::SVirtualMachine(
		const CompiledProgram& program,
		std::shared_ptr<const std::vector<NativeFunctionDefinition>> nativeFunctions)
		: mProgram(program),
		  mNativeFunctions(std::move(nativeFunctions))
	{
	}

	void SVirtualMachine::Execute()
	{
		if(mExecuting)
			throw SException("Virtual machine execution is not reentrant");

		struct ExecutionGuard
		{
			explicit ExecutionGuard(bool& executing)
				: value(executing)
			{
				value = true;
			}

			~ExecutionGuard()
			{
				value = false;
			}

			bool& value;
		};

		ExecutionGuard executionGuard(mExecuting);
		mStack.clear();
		mGlobals.assign(mProgram.globalCount, SValue{});
		mCallFrames.clear();
		mGarbageCollector.Collect({}, {});
		mInstructionPointer = mProgram.entryPoint;
		mNextCollection = 64;

		while(mInstructionPointer < mProgram.instructions.size())
		{
			const Instruction& instruction = mProgram.instructions[mInstructionPointer++];
			try
			{
				switch(instruction.op)
				{
				case OpCode::Add:
				case OpCode::Subtract:
				case OpCode::Multiply:
				case OpCode::Divide:
					{
						double right = RequireNumber(Pop(instruction), instruction);
						double left = RequireNumber(Pop(instruction), instruction);
						if(instruction.op == OpCode::Divide && right == 0.0)
							RuntimeError(instruction, "Division by zero");

						double result = 0.0;
						if(instruction.op == OpCode::Add)
							result = left + right;
						else if(instruction.op == OpCode::Subtract)
							result = left - right;
						else if(instruction.op == OpCode::Multiply)
							result = left * right;
						else
							result = left / right;

						mStack.push_back(SValue::Number(result));
						break;
					}
				case OpCode::Equal:
				case OpCode::NotEqual:
					{
						SValue right = Pop(instruction);
						SValue left = Pop(instruction);
						bool equal = false;

						if(left.Type() == right.Type())
						{
							switch(left.Type())
							{
							case ValueType::Nil:
								equal = true;
								break;
							case ValueType::Boolean:
								equal = left.AsBoolean() == right.AsBoolean();
								break;
							case ValueType::Number:
								equal = left.AsNumber() == right.AsNumber();
								break;
							case ValueType::String:
								equal = left.AsString()->str == right.AsString()->str;
								break;
							case ValueType::Array:
								equal = left.AsArray() == right.AsArray();
								break;
							}
						}

						mStack.push_back(SValue::Boolean(
							instruction.op == OpCode::Equal ? equal : !equal));
						break;
					}
				case OpCode::Less:
				case OpCode::Greater:
				case OpCode::GreaterEqual:
				case OpCode::LessEqual:
					{
						double right = RequireNumber(Pop(instruction), instruction);
						double left = RequireNumber(Pop(instruction), instruction);
						bool result = false;

						if(instruction.op == OpCode::Less)
							result = left < right;
						else if(instruction.op == OpCode::Greater)
							result = left > right;
						else if(instruction.op == OpCode::GreaterEqual)
							result = left >= right;
						else
							result = left <= right;

						mStack.push_back(SValue::Boolean(result));
						break;
					}
				case OpCode::Not:
					mStack.push_back(SValue::Boolean(!Pop(instruction).IsTruthy()));
					break;
				case OpCode::Negate:
					mStack.push_back(SValue::Number(
						-RequireNumber(Pop(instruction), instruction)));
					break;
				case OpCode::Concat:
					{
						SValue right = Pop(instruction);
						SValue left = Pop(instruction);
						if(left.Type() != ValueType::String || right.Type() != ValueType::String)
							RuntimeError(instruction, "Concatenation requires two strings");

						mStack.push_back(CreateManagedString(left.AsString()->str + right.AsString()->str));
						break;
					}
				case OpCode::ToBoolean:
					mStack.push_back(SValue::Boolean(Pop(instruction).IsTruthy()));
					break;
				case OpCode::PushNil:
					mStack.emplace_back();
					break;
				case OpCode::PushBoolean:
					mStack.push_back(SValue::Boolean(instruction.operand != 0));
					break;
				case OpCode::PushNumber:
					mStack.push_back(SValue::Number(instruction.number));
					break;
				case OpCode::PushString:
					mStack.push_back(CreateManagedString(instruction.text));
					break;
				case OpCode::MakeArray:
					{
						if(mStack.size() < instruction.operand)
							RuntimeError(instruction, "Stack underflow while creating array");

						std::size_t start = mStack.size() - instruction.operand;
						std::vector<SValue> values(mStack.begin() + start, mStack.end());
						mStack.resize(start);
						mStack.push_back(CreateManagedArray(std::move(values)));
						break;
					}
				case OpCode::LoadLocal:
					if(mCallFrames.empty())
						RuntimeError(instruction, "Local variable access outside a function");
					if(mCallFrames.back().base + instruction.operand >= mStack.size())
						RuntimeError(instruction, "Invalid local variable slot");
					mStack.push_back(mStack[mCallFrames.back().base + instruction.operand]);
					break;
				case OpCode::StoreLocal:
					if(mCallFrames.empty())
						RuntimeError(instruction, "Local variable access outside a function");
					if(mCallFrames.back().base + instruction.operand >= mStack.size())
						RuntimeError(instruction, "Invalid local variable slot");
					mStack[mCallFrames.back().base + instruction.operand] = Peek(instruction);
					break;
				case OpCode::LoadGlobal:
					if(instruction.operand >= mGlobals.size())
						RuntimeError(instruction, "Invalid global variable slot");
					mStack.push_back(mGlobals[instruction.operand]);
					break;
				case OpCode::StoreGlobal:
					if(instruction.operand >= mGlobals.size())
						RuntimeError(instruction, "Invalid global variable slot");
					mGlobals[instruction.operand] = Peek(instruction);
					break;
				case OpCode::GetIndex:
					{
						std::size_t index = RequireIndex(Pop(instruction), instruction);
						SValue arrayValue = Pop(instruction);
						if(arrayValue.Type() != ValueType::Array)
							RuntimeError(instruction, "Cannot index a non-array value");

						const std::vector<SValue>& array = arrayValue.AsArray()->arr;
						if(index >= array.size())
							RuntimeError(instruction, "Array index out of bounds");

						mStack.push_back(array[index]);
						break;
					}
				case OpCode::SetIndex:
					{
						SValue value = Pop(instruction);
						std::size_t index = RequireIndex(Pop(instruction), instruction);
						SValue arrayValue = Pop(instruction);
						if(arrayValue.Type() != ValueType::Array)
							RuntimeError(instruction, "Cannot index a non-array value");

						std::vector<SValue>& array = arrayValue.MutableArray()->arr;
						if(index >= array.size())
							array.resize(index + 1);
						array[index] = value;
						mStack.push_back(value);
						break;
					}
				case OpCode::Pop:
					Pop(instruction);
					break;
				case OpCode::Jump:
					ValidateJump(instruction.operand, instruction);
					mInstructionPointer = instruction.operand;
					break;
				case OpCode::JumpIfFalse:
					if(!Pop(instruction).IsTruthy())
					{
						ValidateJump(instruction.operand, instruction);
						mInstructionPointer = instruction.operand;
					}
					break;
				case OpCode::JumpIfTrue:
					if(Pop(instruction).IsTruthy())
					{
						ValidateJump(instruction.operand, instruction);
						mInstructionPointer = instruction.operand;
					}
					break;
				case OpCode::Call:
					if(mStack.size() < instruction.operand2)
						RuntimeError(instruction, "Stack underflow while calling function");
					ValidateJump(instruction.operand, instruction);
					mCallFrames.push_back(
						CallFrame{mInstructionPointer, mStack.size() - instruction.operand2});
					mStack.resize(mStack.size() + instruction.operand3);
					mInstructionPointer = instruction.operand;
					break;
				case OpCode::NativeCall:
					{
						if(instruction.operand >= mNativeFunctions->size())
							RuntimeError(instruction, "Invalid native function index");
						if(mStack.size() < instruction.operand2)
							RuntimeError(instruction, "Stack underflow while calling native function");

						std::size_t start = mStack.size() - instruction.operand2;
						std::vector<SValue> rootedArguments;
						rootedArguments.reserve(instruction.operand2);
						for(std::size_t index = start; index < mStack.size(); ++index)
							rootedArguments.push_back(mGarbageCollector.RootValue(mStack[index]));

						std::span<const SValue> arguments(rootedArguments);
						SValue result;
						try
						{
							result = mNativeFunctions->at(instruction.operand).function(*this, arguments);
						}
						catch(const SException&)
						{
							throw;
						}
						catch(const std::exception& exception)
						{
							RuntimeError(
								instruction,
								std::string("Native function failed: ") + exception.what());
						}
						mStack.resize(start);
						mStack.push_back(result.WithoutRoot());
						break;
					}
				case OpCode::Return:
					{
						if(mCallFrames.empty())
							RuntimeError(instruction, "Return executed without a call frame");

						SValue result = Pop(instruction);
						CallFrame frame = mCallFrames.back();
						mCallFrames.pop_back();
						mStack.resize(frame.base);
						mStack.push_back(result);
						mInstructionPointer = frame.returnAddress;
						break;
					}
				case OpCode::Halt:
					mStack.clear();
					mGlobals.clear();
					mCallFrames.clear();
					mGarbageCollector.Collect({}, {});
					return;
				}
			}
			catch(const SException& exception)
			{
				if(exception.HasLocation())
					throw;
				throw SException(instruction.location, exception.Text());
			}

			MaybeCollect();
		}

		throw SException("Instruction pointer left the program without executing HALT");
	}

	SValue SVirtualMachine::CreateString(std::string value)
	{
		return mGarbageCollector.RootValue(
			mGarbageCollector.CreateString(std::move(value)));
	}

	SValue SVirtualMachine::CreateArray(std::vector<SValue> values)
	{
		for(SValue& value : values)
			value = value.WithoutRoot();

		return mGarbageCollector.RootValue(
			mGarbageCollector.CreateArray(std::move(values)));
	}

	SValue SVirtualMachine::Pop(const Instruction& instruction)
	{
		if(mStack.empty())
			RuntimeError(instruction, "Stack underflow");

		SValue value = mStack.back();
		mStack.pop_back();
		return value;
	}

	const SValue& SVirtualMachine::Peek(
		const Instruction& instruction,
		std::size_t distance) const
	{
		if(distance >= mStack.size())
			RuntimeError(instruction, "Stack underflow");

		return mStack[mStack.size() - distance - 1];
	}

	double SVirtualMachine::RequireNumber(
		const SValue& value,
		const Instruction& instruction) const
	{
		if(value.Type() != ValueType::Number)
			RuntimeError(instruction, "Operation requires a number");

		return value.AsNumber();
	}

	std::size_t SVirtualMachine::RequireIndex(
		const SValue& value,
		const Instruction& instruction) const
	{
		double number = RequireNumber(value, instruction);
		if(number < 0.0 || std::floor(number) != number)
			RuntimeError(instruction, "Array index must be a non-negative whole number");
		if(number >= static_cast<double>(std::numeric_limits<std::size_t>::max()))
			RuntimeError(instruction, "Array index is too large");

		return static_cast<std::size_t>(number);
	}

	SValue SVirtualMachine::CreateManagedString(std::string value)
	{
		return mGarbageCollector.CreateString(std::move(value));
	}

	SValue SVirtualMachine::CreateManagedArray(std::vector<SValue> values)
	{
		for(SValue& value : values)
			value = value.WithoutRoot();

		return mGarbageCollector.CreateArray(std::move(values));
	}

	void SVirtualMachine::ValidateJump(
		std::size_t target,
		const Instruction& instruction) const
	{
		if(target >= mProgram.instructions.size())
			RuntimeError(instruction, "Invalid instruction target");
	}

	void SVirtualMachine::MaybeCollect()
	{
		if(mGarbageCollector.ObjectCount() < mNextCollection)
			return;

		mGarbageCollector.Collect(mStack, mGlobals);
		mNextCollection = std::max<std::size_t>(64, mGarbageCollector.ObjectCount() * 2);
	}

	void SVirtualMachine::RuntimeError(
		const Instruction& instruction,
		const std::string& text) const
	{
		throw SException(instruction.location, text);
	}
}
