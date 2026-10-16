#pragma once

#include "SCodeGenerator.h"
#include "SFunctions.h"
#include "SVirtualMachine.h"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace Solstice
{
	class SScript
	{
	public:
		SScript();
		SScript(const SScript&) = delete;
		SScript& operator=(const SScript&) = delete;
		SScript(SScript&&) noexcept = default;
		SScript& operator=(SScript&&) noexcept = default;

		bool Compile(const std::string& fileName, const CompileOptions& options = {});
		bool Execute();
		bool RegisterFunction(
			std::string name,
			std::size_t arity,
			NativeFunction function);

		const std::string& LastError() const noexcept;

	private:
		std::shared_ptr<std::vector<NativeFunctionDefinition>> mNativeFunctions;
		std::unique_ptr<SCodeGenerator> mCodeGenerator;
		std::unique_ptr<SVirtualMachine> mVirtualMachine;
		std::string mLastError;
	};
}
