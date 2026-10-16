#pragma once

#include "SFunctions.h"
#include "SLexicalAnalyzer.h"
#include "SNode.h"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Solstice
{
	struct FuncDefInfo
	{
		std::size_t numArgs = 0;
		std::size_t numLocalVars = 0;
		std::size_t offset = 0;
		SourceLocation location;
	};

	struct NativeFuncInfo
	{
		std::size_t numArgs = 0;
		std::size_t index = 0;
	};

	class SParser
	{
	public:
		explicit SParser(const std::vector<NativeFunctionDefinition>& nativeFunctions);

		NodePtr Parse(const std::string& fileName);

		const std::unordered_map<std::string, FuncDefInfo>& Functions() const noexcept;
		const std::unordered_map<std::string, NativeFuncInfo>& NativeFunctions() const noexcept;
		std::size_t GlobalCount() const noexcept;

	private:
		struct VariableBinding
		{
			int slot = 0;
			bool global = false;
		};

		NodePtr ParseStatement();
		NodePtr ParseFunction();
		NodePtr ParseIf(SourceLocation location);
		NodePtr ParseWhile(SourceLocation location);
		NodePtr ParseFor(SourceLocation location);
		NodePtr ParseRepeat(SourceLocation location);
		NodePtr ParseDo(SourceLocation location);
		NodePtr ParseReturn(SourceLocation location);
		NodePtr ParseLocal(SourceLocation location);
		NodePtr ParseBlock(const std::vector<TOKEN>& terminators, bool createScope);

		NodePtr ParseExpression();
		NodePtr ParseAssignment();
		NodePtr ParseOr();
		NodePtr ParseAnd();
		NodePtr ParseCompare();
		NodePtr ParseConcat();
		NodePtr ParsePlus();
		NodePtr ParseMultiply();
		NodePtr ParseUnary();
		NodePtr ParsePostfix();
		NodePtr ParsePrimary();

		NodePtr CreateVariableNode(const Token& token, bool createIfMissing);
		VariableBinding ResolveVariable(const Token& token, bool createIfMissing);
		VariableBinding DeclareVariable(const Token& token);
		int AllocateTemporary();
		void PushScope();
		void PopScope();

		bool Check(TOKEN type) const noexcept;
		bool Match(TOKEN type);
		Token Consume(TOKEN type, const std::string& expectation);
		bool IsTerminator(const std::vector<TOKEN>& terminators) const;
		void Advance();

		std::unique_ptr<SLexicalAnalyzer> mLexer;
		std::unordered_map<std::string, FuncDefInfo> mFunctions;
		std::unordered_map<std::string, NativeFuncInfo> mNativeFunctions;
		std::unordered_map<std::string, int> mGlobals;
		std::vector<std::unordered_map<std::string, VariableBinding>> mLocalScopes;
		std::size_t mGlobalCount = 0;
		int mNextLocalSlot = 0;
		std::size_t mCurrentArgumentCount = 0;
		bool mInFunction = false;
	};
}
