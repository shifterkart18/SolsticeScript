#include "SNode.h"

#include <array>

namespace Solstice
{
	namespace
	{
		constexpr std::array<const char*, 42> NodeNames =
		{
			"VOID", "ADD", "SUB", "MULT", "DIV", "ASSIGN", "EQUALS",
			"NOTEQUALS", "LESS", "GREATER", "GREATEQUAL", "LESSEQUAL",
			"AND", "OR", "NOT", "NEG", "IF", "ELSE", "ELSEIF", "WHILE",
			"RETURN", "FUNCTION", "VAR", "STRING", "CONCAT", "NUMBER",
			"IDENT", "ARRAY", "INDEX", "FUNCTIONCALL", "REGFUNCCALL",
			"BLOCKSTATEMENT", "PROGRAM", "GLOBAL_IDENT", "GLOBAL_ASSIGN",
			"GLOBAL_INDEX", "NIL", "BOOLEAN", "LOCAL_ASSIGN", "FOR",
			"REPEAT", "DO"
		};
	}

	SNode::SNode(NodeType nodeType, SourceLocation sourceLocation)
		: type(nodeType),
		  location(std::move(sourceLocation))
	{
	}

	SNode::SNode(NodeType nodeType, double number, SourceLocation sourceLocation)
		: type(nodeType),
		  location(std::move(sourceLocation)),
		  numberValue(number)
	{
	}

	SNode::SNode(NodeType nodeType, std::string stringValue, SourceLocation sourceLocation)
		: type(nodeType),
		  location(std::move(sourceLocation)),
		  text(std::move(stringValue))
	{
	}

	void SNode::AddChild(std::unique_ptr<SNode> child)
	{
		children.push_back(std::move(child));
	}

	void SNode::PrintTree(std::ostream& output, int depth) const
	{
		for(int i = 0; i < depth; ++i)
			output << "  ";

		output << NodeNames.at(static_cast<std::size_t>(type));
		if(type == NT_NUMBER)
			output << "  " << numberValue;
		else if(
			type == NT_IDENT ||
			type == NT_GLOBALIDENT ||
			type == NT_FUNCTIONCALL ||
			type == NT_REGFUNCCALL ||
			type == NT_FUNCTION ||
			type == NT_STRING)
		{
			output << "  " << text;
		}
		else if(type == NT_BOOLEAN)
		{
			output << "  " << (boolValue ? "true" : "false");
		}

		output << '\n';
		for(const std::unique_ptr<SNode>& child : children)
			child->PrintTree(output, depth + 1);
	}
}