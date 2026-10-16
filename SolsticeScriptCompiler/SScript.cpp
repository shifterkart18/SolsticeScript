#include "SScript.h"

#include <iostream>
#include <utility>

namespace Solstice
{
	SScript::SScript()
		: mNativeFunctions(
			std::make_shared<std::vector<NativeFunctionDefinition>>(CreateDefaultFunctions())),
		  mCodeGenerator(std::make_unique<SCodeGenerator>(mNativeFunctions))
	{
	}

	bool SScript::Compile(const std::string& fileName, const CompileOptions& options)
	{
		mLastError.clear();
		mVirtualMachine.reset();

		try
		{
			if(!mCodeGenerator->Build(fileName, options))
				return false;

			mVirtualMachine = std::make_unique<SVirtualMachine>(
				mCodeGenerator->Program(),
				mNativeFunctions);
			return true;
		}
		catch(const SException& exception)
		{
			mLastError = exception.message;
			std::cerr << mLastError << '\n';
			return false;
		}
		catch(const std::exception& exception)
		{
			mLastError = std::string("error: ") + exception.what();
			std::cerr << mLastError << '\n';
			return false;
		}
	}

	bool SScript::Execute()
	{
		mLastError.clear();
		if(!mVirtualMachine)
		{
			mLastError = "error: No compiled script is available";
			std::cerr << mLastError << '\n';
			return false;
		}

		try
		{
			mVirtualMachine->Execute();
			return true;
		}
		catch(const SException& exception)
		{
			mLastError = exception.message;
			std::cerr << mLastError << '\n';
			return false;
		}
		catch(const std::exception& exception)
		{
			mLastError = std::string("error: ") + exception.what();
			std::cerr << mLastError << '\n';
			return false;
		}
	}

	bool SScript::RegisterFunction(
		std::string name,
		std::size_t arity,
		NativeFunction function)
	{
		if(!function)
			return false;

		for(const NativeFunctionDefinition& existing : *mNativeFunctions)
		{
			if(existing.name == name)
				return false;
		}

		mNativeFunctions->push_back(
			NativeFunctionDefinition{std::move(name), arity, std::move(function)});
		return true;
	}

	const std::string& SScript::LastError() const noexcept
	{
		return mLastError;
	}
}
