#pragma once

#include "SExceptions.h"

#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace Solstice
{
	enum NodeType
	{
		NT_VOID,
		NT_ADD,
		NT_SUB,
		NT_MULT,
		NT_DIV,
		NT_ASSIGN,
		NT_EQUALS,
		NT_NOTEQUALS,
		NT_LESS,
		NT_GREATER,
		NT_GREATEQUAL,
		NT_LESSEQUAL,
		NT_AND,
		NT_OR,
		NT_NOT,
		NT_NEG,
		NT_IF,
		NT_ELSE,
		NT_ELSEIF,
		NT_WHILE,
		NT_RETURN,
		NT_FUNCTION,
		NT_VAR,
		NT_STRING,
		NT_CONCAT,
		NT_NUMBER,
		NT_IDENT,
		NT_ARRAY,
		NT_INDEX,
		NT_FUNCTIONCALL,
		NT_REGFUNCCALL,
		NT_BLOCKSTATEMENT,
		NT_PROGRAM,
		NT_GLOBALIDENT,
		NT_GLOBALASSIGN,
		NT_GLOBALINDEX,
		NT_NIL,
		NT_BOOLEAN,
		NT_LOCALASSIGN,
		NT_FOR,
		NT_REPEAT,
		NT_DO
	};

	class SNode
	{
	public:
		explicit SNode(NodeType nodeType, SourceLocation sourceLocation = {});
		SNode(NodeType nodeType, double number, SourceLocation sourceLocation = {});
		SNode(NodeType nodeType, std::string stringValue, SourceLocation sourceLocation = {});

		void AddChild(std::unique_ptr<SNode> child);
		void PrintTree(std::ostream& output, int depth = 0) const;

		NodeType type = NT_VOID;
		SourceLocation location;
		std::vector<std::unique_ptr<SNode>> children;
		double numberValue = 0.0;
		std::string text;
		int slot = 0;
		int auxiliarySlot = 0;
		int auxiliarySlot2 = 0;
		bool boolValue = false;
		bool global = false;
	};

	using NodePtr = std::unique_ptr<SNode>;
}