#include "SScript.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
	void WriteScript(const std::filesystem::path& path, const std::string& source)
	{
		std::ofstream output(path, std::ios::binary);
		output << source;
	}

	bool Check(bool condition, const std::string& name)
	{
		std::cout << (condition ? "[PASS] " : "[FAIL] ") << name << '\n';
		return condition;
	}
}

int main()
{
	std::filesystem::path testDirectory =
		std::filesystem::temp_directory_path() /
		(
			"SolsticeScriptTests-" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())
		);
	std::filesystem::create_directories(testDirectory);
	int failures = 0;

	{
		Solstice::SScript script;
		bool argumentsInOrder = false;
		bool registered = script.RegisterFunction(
			"checkargs",
			2,
			[&argumentsInOrder](
				Solstice::SVirtualMachine&,
				std::span<const Solstice::SValue> arguments)
			{
				argumentsInOrder =
					arguments[0].Type() == Solstice::ValueType::Number &&
					arguments[1].Type() == Solstice::ValueType::Number &&
					arguments[0].AsNumber() == 10.0 &&
					arguments[1].AsNumber() == 3.0;
				return Solstice::SValue::Boolean(argumentsInOrder);
			});

		std::filesystem::path scriptPath = testDirectory / "NativeArguments.sun";
		WriteScript(scriptPath, "checkargs(10, 3)\n");
		bool success =
			registered &&
			script.Compile(scriptPath.string()) &&
			script.Execute() &&
			argumentsInOrder;
		if(!Check(success, "native function argument order"))
			++failures;

		if(!Check(
			!script.RegisterFunction("checkargs", 2, {}),
			"duplicate native registration"))
		{
			++failures;
		}

		if(!Check(
			!script.RegisterFunction("empty", 0, {}),
			"empty native registration"))
		{
			++failures;
		}
	}

	{
		Solstice::SScript script;
		std::filesystem::path firstPath = testDirectory / "First.sun";
		std::filesystem::path secondPath = testDirectory / "Second.sun";
		WriteScript(firstPath, "value = \"first\"\n");
		WriteScript(secondPath, "cycle = {}\ncycle[0] = cycle\n");

		bool success =
			script.Compile(firstPath.string()) &&
			script.Execute() &&
			script.Compile(secondPath.string()) &&
			script.Execute();
		if(!Check(success, "repeated compile and execute"))
			++failures;
	}

	{
		Solstice::SScript original;
		Solstice::SScript moved(std::move(original));
		std::filesystem::path scriptPath = testDirectory / "MovedScript.sun";
		WriteScript(scriptPath, "printline(\"moved\")\n");
		bool success = moved.Compile(scriptPath.string()) && moved.Execute();
		if(!Check(success, "moved script ownership"))
			++failures;
	}

	{
		Solstice::SScript script;
		Solstice::SValue retained;
		script.RegisterFunction(
			"capture",
			1,
			[&retained](
				Solstice::SVirtualMachine&,
				std::span<const Solstice::SValue> arguments)
			{
				retained = arguments[0];
				return Solstice::SValue{};
			});

		std::filesystem::path scriptPath = testDirectory / "RetainedNativeValue.sun";
		WriteScript(scriptPath, "capture(\"retained\")\n");
		bool success =
			script.Compile(scriptPath.string()) &&
			script.Execute() &&
			retained.ToString() == "retained";
		if(!Check(success, "native retained value root"))
			++failures;
	}

	{
		Solstice::SScript script;
		Solstice::SValue retainedString;
		Solstice::SValue retainedArray;
		script.RegisterFunction(
			"createvalues",
			0,
			[&retainedString, &retainedArray](
				Solstice::SVirtualMachine& virtualMachine,
				std::span<const Solstice::SValue>)
			{
				retainedString = virtualMachine.CreateString("native-created");
				retainedArray = virtualMachine.CreateArray({retainedString});
				return Solstice::SValue{};
			});

		std::filesystem::path scriptPath = testDirectory / "NativeCreatedValues.sun";
		WriteScript(scriptPath, "createvalues()\n");
		bool success =
			script.Compile(scriptPath.string()) &&
			script.Execute() &&
			retainedString.ToString() == "native-created" &&
			retainedArray.AsArray()->arr[0].ToString() == "native-created";
		if(!Check(success, "native-created retained values"))
			++failures;
	}

	{
		Solstice::SScript script;
		script.RegisterFunction(
			"throwing",
			0,
			[](
				Solstice::SVirtualMachine&,
				std::span<const Solstice::SValue>) -> Solstice::SValue
			{
				throw std::runtime_error("native failure");
			});

		std::filesystem::path scriptPath = testDirectory / "NativeException.sun";
		WriteScript(scriptPath, "throwing()\n");
		bool success =
			script.Compile(scriptPath.string()) &&
			!script.Execute() &&
			script.LastError().find("native failure") != std::string::npos;
		if(!Check(success, "native exception translation"))
			++failures;
	}

	{
		Solstice::SScript script;
		script.RegisterFunction(
			"reenter",
			0,
			[](
				Solstice::SVirtualMachine& virtualMachine,
				std::span<const Solstice::SValue>)
			{
				virtualMachine.Execute();
				return Solstice::SValue{};
			});

		std::filesystem::path scriptPath = testDirectory / "ReentrantExecute.sun";
		WriteScript(scriptPath, "reenter()\n");
		bool success =
			script.Compile(scriptPath.string()) &&
			!script.Execute() &&
			script.LastError().find("not reentrant") != std::string::npos;
		if(!Check(success, "VM reentrancy guard"))
			++failures;
	}

	{
		Solstice::SScript script;
		std::filesystem::path validPath = testDirectory / "Recovery.sun";
		WriteScript(validPath, "value = 42\n");

		bool success =
			!script.Compile((testDirectory / "Missing.sun").string()) &&
			script.Compile(validPath.string()) &&
			script.Execute();
		if(!Check(success, "compile failure recovery"))
			++failures;
	}

	std::filesystem::remove_all(testDirectory);
	return failures == 0 ? 0 : 1;
}
