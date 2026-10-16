#include "SFunctions.h"

#include <iostream>

namespace Solstice
{
	namespace
	{
		SValue Print(SVirtualMachine&, std::span<const SValue> arguments)
		{
			std::cout << arguments[0].ToString();
			return {};
		}

		SValue PrintLine(SVirtualMachine&, std::span<const SValue> arguments)
		{
			std::cout << arguments[0].ToString() << '\n';
			return {};
		}

		SValue Length(SVirtualMachine&, std::span<const SValue> arguments)
		{
			if(arguments[0].Type() != ValueType::Array)
				throw SException("Cannot find the length of a non-array value");

			return SValue::Number(static_cast<double>(arguments[0].AsArray()->arr.size()));
		}
	}

	std::vector<NativeFunctionDefinition> CreateDefaultFunctions()
	{
		return
		{
			{"print", 1, Print},
			{"printline", 1, PrintLine},
			{"length", 1, Length}
		};
	}
}
