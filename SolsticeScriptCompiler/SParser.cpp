#include "SParser.h"

#include <algorithm>
#include <utility>

namespace Solstice
{
	SParser::SParser(const std::vector<NativeFunctionDefinition>& nativeFunctions)
	{
		for(std::size_t index = 0; index < nativeFunctions.size(); ++index)
		{
			const NativeFunctionDefinition& function = nativeFunctions[index];
			mNativeFunctions[function.name] = NativeFuncInfo{function.arity, index};
		}
	}

	NodePtr SParser::Parse(const std::string& fileName)
	{
		mLexer = std::make_unique<SLexicalAnalyzer>(fileName);
		mFunctions.clear();
		mGlobals.clear();
		mLocalScopes.clear();
		mGlobalCount = 0;
		mNextLocalSlot = 0;
		mCurrentArgumentCount = 0;
		mInFunction = false;
		PushScope();

		auto program = std::make_unique<SNode>(NT_PROGRAM, mLexer->Current().location);
		while(!Check(TOKEN_EOF))
		{
			if(Match(TOKEN_FUNCTION))
				program->AddChild(ParseFunction());
			else
				program->AddChild(ParseStatement());
		}

		return program;
	}

	const std::unordered_map<std::string, FuncDefInfo>& SParser::Functions() const noexcept
	{
		return mFunctions;
	}

	const std::unordered_map<std::string, NativeFuncInfo>& SParser::NativeFunctions() const noexcept
	{
		return mNativeFunctions;
	}

	std::size_t SParser::GlobalCount() const noexcept
	{
		return mGlobalCount;
	}

	NodePtr SParser::ParseStatement()
	{
		SourceLocation location = mLexer->Current().location;

		if(Match(TOKEN_IF))
			return ParseIf(location);
		if(Match(TOKEN_WHILE))
			return ParseWhile(location);
		if(Match(TOKEN_FOR))
			return ParseFor(location);
		if(Match(TOKEN_REPEAT))
			return ParseRepeat(location);
		if(Match(TOKEN_DO))
			return ParseDo(location);
		if(Match(TOKEN_RETURN))
			return ParseReturn(location);
		if(Match(TOKEN_LOCAL))
			return ParseLocal(location);
		if(Check(TOKEN_FUNCTION))
			throw SException(location, "Functions may only be declared at the top level");

		auto statement = std::make_unique<SNode>(NT_VOID, location);
		statement->AddChild(ParseExpression());
		return statement;
	}

	NodePtr SParser::ParseFunction()
	{
		Token name = Consume(TOKEN_IDENT, "function name");
		if(mFunctions.contains(name.text))
			throw SException(name.location, "Redeclaration of function '" + name.text + "'");
		if(mNativeFunctions.contains(name.text))
			throw SException(name.location, "Function name conflicts with native function '" + name.text + "'");

		Consume(TOKEN_LPAREN, "'('");

		auto outerScopes = std::move(mLocalScopes);
		mInFunction = true;
		mNextLocalSlot = 0;
		mCurrentArgumentCount = 0;
		mLocalScopes.clear();
		PushScope();

		if(!Check(TOKEN_RPAREN))
		{
			do
			{
				Token argument = Consume(TOKEN_IDENT, "argument name");
				if(mLocalScopes.back().contains(argument.text))
					throw SException(argument.location, "Duplicate argument '" + argument.text + "'");

				mLocalScopes.back()[argument.text] = VariableBinding{mNextLocalSlot++, false};
				++mCurrentArgumentCount;
			}
			while(Match(TOKEN_COMMA));
		}

		Consume(TOKEN_RPAREN, "')'");
		auto body = ParseBlock({TOKEN_END}, false);
		Consume(TOKEN_END, "'end'");

		FuncDefInfo information;
		information.numArgs = mCurrentArgumentCount;
		information.numLocalVars =
			static_cast<std::size_t>(mNextLocalSlot) - mCurrentArgumentCount;
		information.location = name.location;
		mFunctions[name.text] = information;

		auto function = std::make_unique<SNode>(NT_FUNCTION, name.text, name.location);
		function->AddChild(std::move(body));

		mLocalScopes = std::move(outerScopes);
		mNextLocalSlot = 0;
		mCurrentArgumentCount = 0;
		mInFunction = false;
		return function;
	}

	NodePtr SParser::ParseIf(SourceLocation location)
	{
		auto node = std::make_unique<SNode>(NT_IF, location);
		node->AddChild(ParseExpression());
		Consume(TOKEN_THEN, "'then'");
		node->AddChild(ParseBlock({TOKEN_ELSEIF, TOKEN_ELSE, TOKEN_END}, true));

		if(Match(TOKEN_ELSEIF))
		{
			node->AddChild(ParseIf(mLexer->Current().location));
		}
		else if(Match(TOKEN_ELSE))
		{
			node->AddChild(ParseBlock({TOKEN_END}, true));
			Consume(TOKEN_END, "'end'");
		}
		else
		{
			Consume(TOKEN_END, "'end'");
		}

		return node;
	}

	NodePtr SParser::ParseWhile(SourceLocation location)
	{
		auto node = std::make_unique<SNode>(NT_WHILE, location);
		node->AddChild(ParseExpression());
		Consume(TOKEN_DO, "'do'");
		node->AddChild(ParseBlock({TOKEN_END}, true));
		Consume(TOKEN_END, "'end'");
		return node;
	}

	NodePtr SParser::ParseFor(SourceLocation location)
	{
		Token name = Consume(TOKEN_IDENT, "loop variable");
		Consume(TOKEN_ASSIGN, "'='");

		auto start = ParseExpression();
		Consume(TOKEN_COMMA, "','");
		auto end = ParseExpression();

		NodePtr step;
		if(Match(TOKEN_COMMA))
			step = ParseExpression();
		else
			step = std::make_unique<SNode>(NT_NUMBER, 1.0, location);

		Consume(TOKEN_DO, "'do'");
		PushScope();
		VariableBinding loopVariable = DeclareVariable(name);
		int endSlot = AllocateTemporary();
		int stepSlot = AllocateTemporary();
		auto body = ParseBlock({TOKEN_END}, false);
		Consume(TOKEN_END, "'end'");
		PopScope();

		auto node = std::make_unique<SNode>(NT_FOR, location);
		node->slot = loopVariable.slot;
		node->global = loopVariable.global;
		node->auxiliarySlot = endSlot;
		node->auxiliarySlot2 = stepSlot;
		node->AddChild(std::move(start));
		node->AddChild(std::move(end));
		node->AddChild(std::move(step));
		node->AddChild(std::move(body));
		return node;
	}

	NodePtr SParser::ParseRepeat(SourceLocation location)
	{
		PushScope();
		auto body = ParseBlock({TOKEN_UNTIL}, false);
		Consume(TOKEN_UNTIL, "'until'");
		auto condition = ParseExpression();
		PopScope();

		auto node = std::make_unique<SNode>(NT_REPEAT, location);
		node->AddChild(std::move(body));
		node->AddChild(std::move(condition));
		return node;
	}

	NodePtr SParser::ParseDo(SourceLocation location)
	{
		auto node = std::make_unique<SNode>(NT_DO, location);
		node->AddChild(ParseBlock({TOKEN_END}, true));
		Consume(TOKEN_END, "'end'");
		return node;
	}

	NodePtr SParser::ParseReturn(SourceLocation location)
	{
		if(!mInFunction)
			throw SException(location, "Cannot return outside of a function");

		auto node = std::make_unique<SNode>(NT_RETURN, location);
		if(
			!Check(TOKEN_END) &&
			!Check(TOKEN_ELSE) &&
			!Check(TOKEN_ELSEIF) &&
			!Check(TOKEN_UNTIL) &&
			!Check(TOKEN_EOF) &&
			mLexer->Current().location.line == location.line)
		{
			node->AddChild(ParseExpression());
		}

		return node;
	}

	NodePtr SParser::ParseLocal(SourceLocation location)
	{
		Token name = Consume(TOKEN_IDENT, "local variable name");
		NodePtr initializer;
		if(Match(TOKEN_ASSIGN))
			initializer = ParseExpression();
		else
			initializer = std::make_unique<SNode>(NT_NIL, location);

		VariableBinding binding = DeclareVariable(name);
		auto variable = std::make_unique<SNode>(
			binding.global ? NT_GLOBALIDENT : NT_IDENT,
			name.text,
			name.location);
		variable->slot = binding.slot;

		auto assignment = std::make_unique<SNode>(NT_LOCALASSIGN, location);
		assignment->AddChild(std::move(variable));
		assignment->AddChild(std::move(initializer));
		return assignment;
	}

	NodePtr SParser::ParseBlock(const std::vector<TOKEN>& terminators, bool createScope)
	{
		if(createScope)
			PushScope();

		auto block = std::make_unique<SNode>(NT_BLOCKSTATEMENT, mLexer->Current().location);
		while(!Check(TOKEN_EOF) && !IsTerminator(terminators))
			block->AddChild(ParseStatement());

		if(Check(TOKEN_EOF) && !IsTerminator(terminators))
			throw SException(mLexer->Current().location, "Unexpected end of file");

		if(createScope)
			PopScope();

		return block;
	}

	NodePtr SParser::ParseExpression()
	{
		return ParseAssignment();
	}

	NodePtr SParser::ParseAssignment()
	{
		NodePtr left = ParseOr();
		if(!Match(TOKEN_ASSIGN))
			return left;

		SourceLocation location = left->location;
		if(left->type != NT_IDENT && left->type != NT_GLOBALIDENT && left->type != NT_INDEX)
			throw SException(location, "Invalid assignment target");

		auto assignment = std::make_unique<SNode>(NT_ASSIGN, location);
		assignment->AddChild(std::move(left));
		assignment->AddChild(ParseAssignment());
		return assignment;
	}

	NodePtr SParser::ParseOr()
	{
		NodePtr node = ParseAnd();
		while(Match(TOKEN_OR))
		{
			auto operation = std::make_unique<SNode>(NT_OR, node->location);
			operation->AddChild(std::move(node));
			operation->AddChild(ParseAnd());
			node = std::move(operation);
		}
		return node;
	}

	NodePtr SParser::ParseAnd()
	{
		NodePtr node = ParseCompare();
		while(Match(TOKEN_AND))
		{
			auto operation = std::make_unique<SNode>(NT_AND, node->location);
			operation->AddChild(std::move(node));
			operation->AddChild(ParseCompare());
			node = std::move(operation);
		}
		return node;
	}

	NodePtr SParser::ParseCompare()
	{
		NodePtr node = ParseConcat();
		while(
			Check(TOKEN_LESS) ||
			Check(TOKEN_GREATER) ||
			Check(TOKEN_EQUAL) ||
			Check(TOKEN_NOTEQUAL) ||
			Check(TOKEN_GREATEQUAL) ||
			Check(TOKEN_LESSEQUAL))
		{
			TOKEN operation = mLexer->Current().type;
			SourceLocation location = mLexer->Current().location;
			Advance();

			NodeType type = NT_EQUALS;
			switch(operation)
			{
			case TOKEN_LESS:
				type = NT_LESS;
				break;
			case TOKEN_GREATER:
				type = NT_GREATER;
				break;
			case TOKEN_NOTEQUAL:
				type = NT_NOTEQUALS;
				break;
			case TOKEN_GREATEQUAL:
				type = NT_GREATEQUAL;
				break;
			case TOKEN_LESSEQUAL:
				type = NT_LESSEQUAL;
				break;
			default:
				break;
			}

			auto comparison = std::make_unique<SNode>(type, location);
			comparison->AddChild(std::move(node));
			comparison->AddChild(ParseConcat());
			node = std::move(comparison);
		}
		return node;
	}

	NodePtr SParser::ParseConcat()
	{
		NodePtr node = ParsePlus();
		if(Match(TOKEN_CONCAT))
		{
			auto operation = std::make_unique<SNode>(NT_CONCAT, node->location);
			operation->AddChild(std::move(node));
			operation->AddChild(ParseConcat());
			return operation;
		}
		return node;
	}

	NodePtr SParser::ParsePlus()
	{
		NodePtr node = ParseMultiply();
		while(Check(TOKEN_PLUS) || Check(TOKEN_MINUS))
		{
			TOKEN operation = mLexer->Current().type;
			SourceLocation location = mLexer->Current().location;
			Advance();

			auto expression = std::make_unique<SNode>(
				operation == TOKEN_PLUS ? NT_ADD : NT_SUB,
				location);
			expression->AddChild(std::move(node));
			expression->AddChild(ParseMultiply());
			node = std::move(expression);
		}
		return node;
	}

	NodePtr SParser::ParseMultiply()
	{
		NodePtr node = ParseUnary();
		while(Check(TOKEN_MULT) || Check(TOKEN_DIV))
		{
			TOKEN operation = mLexer->Current().type;
			SourceLocation location = mLexer->Current().location;
			Advance();

			auto expression = std::make_unique<SNode>(
				operation == TOKEN_MULT ? NT_MULT : NT_DIV,
				location);
			expression->AddChild(std::move(node));
			expression->AddChild(ParseUnary());
			node = std::move(expression);
		}
		return node;
	}

	NodePtr SParser::ParseUnary()
	{
		if(Check(TOKEN_MINUS) || Check(TOKEN_NOT))
		{
			TOKEN operation = mLexer->Current().type;
			SourceLocation location = mLexer->Current().location;
			Advance();

			auto node = std::make_unique<SNode>(
				operation == TOKEN_MINUS ? NT_NEG : NT_NOT,
				location);
			node->AddChild(ParseUnary());
			return node;
		}

		return ParsePostfix();
	}

	NodePtr SParser::ParsePostfix()
	{
		NodePtr node = ParsePrimary();
		while(Match(TOKEN_LBRACK))
		{
			auto index = std::make_unique<SNode>(NT_INDEX, node->location);
			index->AddChild(std::move(node));
			index->AddChild(ParseExpression());
			Consume(TOKEN_RBRACK, "']'");
			node = std::move(index);
		}
		return node;
	}

	NodePtr SParser::ParsePrimary()
	{
		Token token = mLexer->Current();
		if(Match(TOKEN_NUMBER))
			return std::make_unique<SNode>(NT_NUMBER, token.number, token.location);
		if(Match(TOKEN_STRING))
			return std::make_unique<SNode>(NT_STRING, token.text, token.location);
		if(Match(TOKEN_TRUE) || Match(TOKEN_FALSE))
		{
			auto value = std::make_unique<SNode>(NT_BOOLEAN, token.location);
			value->boolValue = token.type == TOKEN_TRUE;
			return value;
		}
		if(Match(TOKEN_NIL))
			return std::make_unique<SNode>(NT_NIL, token.location);
		if(Match(TOKEN_LPAREN))
		{
			NodePtr expression = ParseExpression();
			Consume(TOKEN_RPAREN, "')'");
			return expression;
		}
		if(Match(TOKEN_LCURLY))
		{
			auto array = std::make_unique<SNode>(NT_ARRAY, token.location);
			if(!Check(TOKEN_RCURLY))
			{
				do
				{
					array->AddChild(ParseExpression());
				}
				while(Match(TOKEN_COMMA));
			}
			Consume(TOKEN_RCURLY, "'}'");
			return array;
		}
		if(Match(TOKEN_IDENT))
		{
			if(Match(TOKEN_LPAREN))
			{
				bool native = mNativeFunctions.contains(token.text);
				auto call = std::make_unique<SNode>(
					native ? NT_REGFUNCCALL : NT_FUNCTIONCALL,
					token.text,
					token.location);

				if(!Check(TOKEN_RPAREN))
				{
					do
					{
						call->AddChild(ParseExpression());
					}
					while(Match(TOKEN_COMMA));
				}
				Consume(TOKEN_RPAREN, "')'");

				if(native)
				{
					const NativeFuncInfo& info = mNativeFunctions.at(token.text);
					if(call->children.size() != info.numArgs)
					{
						throw SException(
							token.location,
							"Native function '" + token.text + "' expects " +
								std::to_string(info.numArgs) + " argument(s)");
					}
				}

				return call;
			}

			return CreateVariableNode(token, Check(TOKEN_ASSIGN));
		}

		throw SException(token.location, "Expected an expression");
	}

	NodePtr SParser::CreateVariableNode(const Token& token, bool createIfMissing)
	{
		VariableBinding binding = ResolveVariable(token, createIfMissing);
		auto node = std::make_unique<SNode>(
			binding.global ? NT_GLOBALIDENT : NT_IDENT,
			token.text,
			token.location);
		node->slot = binding.slot;
		return node;
	}

	SParser::VariableBinding SParser::ResolveVariable(const Token& token, bool createIfMissing)
	{
		if(mInFunction)
		{
			for(auto scope = mLocalScopes.rbegin(); scope != mLocalScopes.rend(); ++scope)
			{
				auto variable = scope->find(token.text);
				if(variable != scope->end())
					return variable->second;
			}

			auto global = mGlobals.find(token.text);
			if(global != mGlobals.end())
				return VariableBinding{global->second, true};

			if(createIfMissing)
				return DeclareVariable(token);
		}
		else
		{
			for(auto scope = mLocalScopes.rbegin(); scope != mLocalScopes.rend(); ++scope)
			{
				auto variable = scope->find(token.text);
				if(variable != scope->end())
					return variable->second;
			}

			auto global = mGlobals.find(token.text);
			if(global != mGlobals.end())
				return VariableBinding{global->second, true};

			if(createIfMissing)
			{
				int slot = static_cast<int>(mGlobalCount++);
				mGlobals[token.text] = slot;
				return VariableBinding{slot, true};
			}
		}

		throw SException(token.location, "Undefined variable '" + token.text + "'");
	}

	SParser::VariableBinding SParser::DeclareVariable(const Token& token)
	{
		if(mLocalScopes.empty())
			PushScope();
		if(mLocalScopes.back().contains(token.text))
			throw SException(token.location, "Redeclaration of local variable '" + token.text + "'");

		VariableBinding binding;
		if(mInFunction)
			binding = VariableBinding{mNextLocalSlot++, false};
		else
			binding = VariableBinding{static_cast<int>(mGlobalCount++), true};

		mLocalScopes.back()[token.text] = binding;
		return binding;
	}

	int SParser::AllocateTemporary()
	{
		if(mInFunction)
			return mNextLocalSlot++;

		return static_cast<int>(mGlobalCount++);
	}

	void SParser::PushScope()
	{
		mLocalScopes.emplace_back();
	}

	void SParser::PopScope()
	{
		mLocalScopes.pop_back();
	}

	bool SParser::Check(TOKEN type) const noexcept
	{
		return mLexer->Current().type == type;
	}

	bool SParser::Match(TOKEN type)
	{
		if(!Check(type))
			return false;

		Advance();
		return true;
	}

	Token SParser::Consume(TOKEN type, const std::string& expectation)
	{
		if(!Check(type))
			throw SException(mLexer->Current().location, "Expected " + expectation);

		Token token = mLexer->Current();
		Advance();
		return token;
	}

	bool SParser::IsTerminator(const std::vector<TOKEN>& terminators) const
	{
		return std::find(terminators.begin(), terminators.end(), mLexer->Current().type) != terminators.end();
	}

	void SParser::Advance()
	{
		mLexer->Next();
	}
}
