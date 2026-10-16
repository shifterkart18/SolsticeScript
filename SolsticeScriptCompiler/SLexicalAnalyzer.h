#pragma once

#include "SExceptions.h"

#include <string>
#include <unordered_map>

namespace Solstice
{
	enum TOKEN
	{
		TOKEN_EOF = 0,
		TOKEN_FUNCTION,
		TOKEN_RETURN,
		TOKEN_IF,
		TOKEN_THEN,
		TOKEN_ELSE,
		TOKEN_ELSEIF,
		TOKEN_END,
		TOKEN_LOCAL,
		TOKEN_TRUE,
		TOKEN_FALSE,
		TOKEN_FOR,
		TOKEN_WHILE,
		TOKEN_REPEAT,
		TOKEN_UNTIL,
		TOKEN_NIL,
		TOKEN_ASSIGN,
		TOKEN_PLUS,
		TOKEN_MINUS,
		TOKEN_MULT,
		TOKEN_DIV,
		TOKEN_AND,
		TOKEN_OR,
		TOKEN_NOT,
		TOKEN_RPAREN,
		TOKEN_LPAREN,
		TOKEN_RCURLY,
		TOKEN_LCURLY,
		TOKEN_RBRACK,
		TOKEN_LBRACK,
		TOKEN_COMMA,
		TOKEN_GREATER,
		TOKEN_LESS,
		TOKEN_EQUAL,
		TOKEN_NOTEQUAL,
		TOKEN_GREATEQUAL,
		TOKEN_LESSEQUAL,
		TOKEN_CONCAT,
		TOKEN_DO,
		TOKEN_NUMBER,
		TOKEN_IDENT,
		TOKEN_STRING,
	};

	struct Token
	{
		TOKEN type = TOKEN_EOF;
		SourceLocation location;
		std::string text;
		double number = 0.0;
	};

	class SLexicalAnalyzer
	{
	public:
		explicit SLexicalAnalyzer(const std::string& fileName);

		void Next();
		const Token& Current() const noexcept;
		void DisplayTokens() const;

	private:
		char Advance();
		bool AtEnd() const noexcept;
		bool Match(char expected);
		char Peek(std::size_t offset = 0) const noexcept;
		void SkipTrivia();
		Token ScanIdentifier(SourceLocation location);
		Token ScanNumber(SourceLocation location, char first);
		Token ScanString(SourceLocation location);
		Token MakeToken(TOKEN type, SourceLocation location, std::string text = {}) const;
		SourceLocation Location() const;

		std::string mFileName;
		std::string mSource;
		std::size_t mCurrent = 0;
		std::size_t mLine = 1;
		std::size_t mColumn = 1;
		Token mToken;
		std::unordered_map<std::string, TOKEN> mKeywords;
	};
}
