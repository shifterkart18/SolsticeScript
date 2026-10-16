#include "SLexicalAnalyzer.h"

#include <cctype>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <utility>

namespace Solstice
{
	SLexicalAnalyzer::SLexicalAnalyzer(const std::string& fileName)
		: mFileName(fileName)
	{
		std::ifstream input(fileName, std::ios::binary);
		if(!input)
			throw SException("Unable to open source file '" + fileName + "'");

		mSource.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());

		mKeywords["return"] =	TOKEN_RETURN;
		mKeywords["function"] = TOKEN_FUNCTION;
		mKeywords["if"] =		TOKEN_IF;
		mKeywords["then"] =		TOKEN_THEN;
		mKeywords["else"] =		TOKEN_ELSE;
		mKeywords["elseif"] =	TOKEN_ELSEIF;
		mKeywords["end"] =		TOKEN_END;
		mKeywords["do"] =		TOKEN_DO;
		mKeywords["local"] =	TOKEN_LOCAL;
		mKeywords["true"] =		TOKEN_TRUE;
		mKeywords["false"] =	TOKEN_FALSE;
		mKeywords["for"] =		TOKEN_FOR;
		mKeywords["while"] =	TOKEN_WHILE;
		mKeywords["repeat"] =	TOKEN_REPEAT;
		mKeywords["until"] =	TOKEN_UNTIL;
		mKeywords["nil"] =		TOKEN_NIL;
		mKeywords["not"] =		TOKEN_NOT;
		mKeywords["and"] =		TOKEN_AND;
		mKeywords["or"] =		TOKEN_OR;
		Next();
	}

	void SLexicalAnalyzer::Next()
	{
		SkipTrivia();
		SourceLocation location = Location();

		if(AtEnd())
		{
			mToken = MakeToken(TOKEN_EOF, location);
			return;
		}

		char value = Advance();
		switch(value)
		{
		case '(':
			mToken = MakeToken(TOKEN_LPAREN, location, "(");
			return;
		case ')':
			mToken = MakeToken(TOKEN_RPAREN, location, ")");
			return;
		case '[':
			mToken = MakeToken(TOKEN_LBRACK, location, "[");
			return;
		case ']':
			mToken = MakeToken(TOKEN_RBRACK, location, "]");
			return;
		case '{':
			mToken = MakeToken(TOKEN_LCURLY, location, "{");
			return;
		case '}':
			mToken = MakeToken(TOKEN_RCURLY, location, "}");
			return;
		case '+':
			mToken = MakeToken(TOKEN_PLUS, location, "+");
			return;
		case '-':
			mToken = MakeToken(TOKEN_MINUS, location, "-");
			return;
		case '*':
			mToken = MakeToken(TOKEN_MULT, location, "*");
			return;
		case '/':
			mToken = MakeToken(TOKEN_DIV, location, "/");
			return;
		case ',':
			mToken = MakeToken(TOKEN_COMMA, location, ",");
			return;
		case '=':
			if(Match('='))
				mToken = MakeToken(TOKEN_EQUAL, location, "==");
			else
				mToken = MakeToken(TOKEN_ASSIGN, location, "=");
			return;
		case '>':
			if(Match('='))
				mToken = MakeToken(TOKEN_GREATEQUAL, location, ">=");
			else
				mToken = MakeToken(TOKEN_GREATER, location, ">");
			return;
		case '<':
			if(Match('='))
				mToken = MakeToken(TOKEN_LESSEQUAL, location, "<=");
			else
				mToken = MakeToken(TOKEN_LESS, location, "<");
			return;
		case '~':
			if(!Match('='))
				throw SException(location, "Expected '=' after '~'");
			mToken = MakeToken(TOKEN_NOTEQUAL, location, "~=");
			return;
		case '.':
			if(Match('.'))
			{
				mToken = MakeToken(TOKEN_CONCAT, location, "..");
				return;
			}
			if(std::isdigit(static_cast<unsigned char>(Peek())))
			{
				mToken = ScanNumber(location, value);
				return;
			}
			throw SException(location, "Unexpected '.'");
		case '"':
			mToken = ScanString(location);
			return;
		default:
			if(std::isdigit(static_cast<unsigned char>(value)))
			{
				mToken = ScanNumber(location, value);
				return;
			}
			if(std::isalpha(static_cast<unsigned char>(value)) || value == '_')
			{
				--mCurrent;
				--mColumn;
				mToken = ScanIdentifier(location);
				return;
			}

			throw SException(location, std::string("Invalid symbol '") + value + "'");
		}
	}

	const Token& SLexicalAnalyzer::Current() const noexcept
	{
		return mToken;
	}

	void SLexicalAnalyzer::DisplayTokens() const
	{
		SLexicalAnalyzer lexer(mFileName);
		do
		{
			const Token& token = lexer.Current();
			std::cout << token.location.line << ':' << token.location.column << '\t'
				<< static_cast<int>(token.type) << '\t' << token.text << '\n';
			lexer.Next();
		}
		while(lexer.Current().type != TOKEN_EOF);
	}

	char SLexicalAnalyzer::Advance()
	{
		char value = mSource[mCurrent++];
		if(value == '\n')
		{
			++mLine;
			mColumn = 1;
		}
		else
		{
			++mColumn;
		}

		return value;
	}

	bool SLexicalAnalyzer::AtEnd() const noexcept
	{
		return mCurrent >= mSource.size();
	}

	bool SLexicalAnalyzer::Match(char expected)
	{
		if(expected == '\0' || AtEnd() || mSource[mCurrent] != expected)
			return false;

		Advance();
		return true;
	}

	char SLexicalAnalyzer::Peek(std::size_t offset) const noexcept
	{
		std::size_t index = mCurrent + offset;
		return index < mSource.size() ? mSource[index] : '\0';
	}

	void SLexicalAnalyzer::SkipTrivia()
	{
		while(!AtEnd())
		{
			char value = Peek();
			if(value == ' ' || value == '\t' || value == '\r' || value == '\n')
			{
				Advance();
				continue;
			}

			if((value == '-' && Peek(1) == '-') || (value == '/' && Peek(1) == '/'))
			{
				while(!AtEnd() && Peek() != '\n')
					Advance();
				continue;
			}

			break;
		}
	}

	Token SLexicalAnalyzer::ScanIdentifier(SourceLocation location)
	{
		std::size_t start = mCurrent;
		while(std::isalnum(static_cast<unsigned char>(Peek())) || Peek() == '_')
			Advance();

		std::string text = mSource.substr(start, mCurrent - start);
		auto keyword = mKeywords.find(text);
		return MakeToken(keyword == mKeywords.end() ? TOKEN_IDENT : keyword->second, location, text);
	}

	Token SLexicalAnalyzer::ScanNumber(SourceLocation location, char first)
	{
		std::string text(1, first);
		while(std::isdigit(static_cast<unsigned char>(Peek())))
			text += Advance();

		if(Peek() == '.' && Peek(1) != '.')
		{
			text += Advance();
			if(!std::isdigit(static_cast<unsigned char>(Peek())))
				throw SException(location, "Expected a digit after the decimal point");

			while(std::isdigit(static_cast<unsigned char>(Peek())))
				text += Advance();
		}

		Token token = MakeToken(TOKEN_NUMBER, location, text);
		try
		{
			token.number = std::stod(text);
		}
		catch(const std::invalid_argument&)
		{
			throw SException(location, "Invalid numeric literal");
		}
		catch(const std::out_of_range&)
		{
			throw SException(location, "Numeric literal is outside the supported range");
		}
		return token;
	}

	Token SLexicalAnalyzer::ScanString(SourceLocation location)
	{
		std::string value;
		while(!AtEnd() && Peek() != '"')
		{
			char character = Advance();
			if(character == '\n')
				throw SException(location, "Unterminated string literal");

			if(character == '\\')
			{
				if(AtEnd())
					throw SException(location, "Unterminated string escape");

				switch(Advance())
				{
				case 'n':
					value += '\n';
					break;
				case 'r':
					value += '\r';
					break;
				case 't':
					value += '\t';
					break;
				case '"':
					value += '"';
					break;
				case '\\':
					value += '\\';
					break;
				default:
					throw SException(location, "Unsupported string escape");
				}
			}
			else
			{
				value += character;
			}
		}

		if(AtEnd())
			throw SException(location, "Unterminated string literal");

		Advance();
		return MakeToken(TOKEN_STRING, location, std::move(value));
	}

	Token SLexicalAnalyzer::MakeToken(TOKEN type, SourceLocation location, std::string text) const
	{
		return Token{type, std::move(location), std::move(text), 0.0};
	}

	SourceLocation SLexicalAnalyzer::Location() const
	{
		return SourceLocation{mFileName, mLine, mColumn};
	}
}
