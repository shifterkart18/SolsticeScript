#include "SScript.h"

#include <iostream>
#include <string>

namespace
{
	void PrintUsage()
	{
		std::cout
			<< "Usage: SolsticeScriptCompiler [options] <script.sun>\n"
			<< "Options:\n"
			<< "  --compile-only    Compile without executing\n"
			<< "  --dump-tokens     Print lexer tokens\n"
			<< "  --dump-ast        Print the parsed syntax tree\n"
			<< "  --dump-bytecode   Print generated instructions\n"
			<< "  --help            Show this help\n";
	}
}

int main(int argc, char* argv[])
{
	Solstice::CompileOptions options;
	bool compileOnly = false;
	std::string scriptFile;

	for(int index = 1; index < argc; ++index)
	{
		std::string argument = argv[index];
		if(argument == "--help")
		{
			PrintUsage();
			return 0;
		}
		if(argument == "--compile-only")
			compileOnly = true;
		else if(argument == "--dump-tokens")
			options.dumpTokens = true;
		else if(argument == "--dump-ast")
			options.dumpAst = true;
		else if(argument == "--dump-bytecode")
			options.dumpBytecode = true;
		else if(!argument.empty() && argument[0] == '-')
		{
			std::cerr << "Unknown option: " << argument << '\n';
			return 64;
		}
		else if(scriptFile.empty())
			scriptFile = std::move(argument);
		else
		{
			std::cerr << "Only one script file may be specified\n";
			return 64;
		}
	}

	if(scriptFile.empty())
	{
		PrintUsage();
		return 64;
	}

	Solstice::SScript script;
	if(!script.Compile(scriptFile, options))
		return 1;
	if(!compileOnly && !script.Execute())
		return 2;

	return 0;
}