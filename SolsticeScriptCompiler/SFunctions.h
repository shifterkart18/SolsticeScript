#pragma once

#include "SValue.h"
#include "SExceptions.h"

#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <vector>

namespace Solstice
{
	class SVirtualMachine;

	using NativeFunction = std::function<SValue(SVirtualMachine&, std::span<const SValue>)>;

	struct NativeFunctionDefinition
	{
		std::string name;
		std::size_t arity = 0;
		NativeFunction function;
	};

	std::vector<NativeFunctionDefinition> CreateDefaultFunctions();
}