#pragma once

#include "SCodeGenerator.h"
#include "SFunctions.h"
#include "SValue.h"

#include <cstddef>
#include <span>
#include <vector>

namespace Solstice
{
	class SVirtualMachine
	{
	public:
		SVirtualMachine(
			const CompiledProgram& program,
			std::shared_ptr<const std::vector<NativeFunctionDefinition>> nativeFunctions);

		void Execute();
		SValue CreateString(std::string value);
		SValue CreateArray(std::vector<SValue> values = {});

	private:
		struct CallFrame
		{
			std::size_t returnAddress = 0;
			std::size_t base = 0;
		};

		SValue Pop(const Instruction& instruction);
		const SValue& Peek(const Instruction& instruction, std::size_t distance = 0) const;
		double RequireNumber(const SValue& value, const Instruction& instruction) const;
		std::size_t RequireIndex(const SValue& value, const Instruction& instruction) const;
		SValue CreateManagedString(std::string value);
		SValue CreateManagedArray(std::vector<SValue> values);
		void ValidateJump(std::size_t target, const Instruction& instruction) const;
		void MaybeCollect();
		[[noreturn]] void RuntimeError(const Instruction& instruction, const std::string& text) const;

		CompiledProgram mProgram;
		std::shared_ptr<const std::vector<NativeFunctionDefinition>> mNativeFunctions;
		GarbageCollector mGarbageCollector;
		std::vector<SValue> mStack;
		std::vector<SValue> mGlobals;
		std::vector<CallFrame> mCallFrames;
		std::size_t mInstructionPointer = 0;
		std::size_t mNextCollection = 64;
		bool mExecuting = false;
	};
}