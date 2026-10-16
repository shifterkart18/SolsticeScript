#include "SExceptions.h"

#include <sstream>

namespace Solstice
{
	SException::SException(std::string text)
		: std::runtime_error(Format({}, text)),
		  message(Format({}, text)),
		  mText(std::move(text))
	{
	}

	SException::SException(SourceLocation location, std::string text)
		: std::runtime_error(Format(location, text)),
		  message(Format(location, text)),
		  mLocation(std::move(location)),
		  mText(std::move(text))
	{
	}

	SException::SException(int lineNumber, std::string text)
		: SException(SourceLocation{"", static_cast<std::size_t>(lineNumber), 0}, std::move(text))
	{
	}

	const SourceLocation& SException::Location() const noexcept
	{
		return mLocation;
	}

	bool SException::HasLocation() const noexcept
	{
		return mLocation.IsValid();
	}

	const std::string& SException::Text() const noexcept
	{
		return mText;
	}

	std::string SException::Format(const SourceLocation& location, const std::string& text)
	{
		std::ostringstream output;

		if(location.IsValid())
		{
			if(!location.source.empty())
				output << location.source << ':';

			output << location.line;
			if(location.column != 0)
				output << ':' << location.column;

			output << ": ";
		}

		output << "error: " << text;
		return output.str();
	}
}