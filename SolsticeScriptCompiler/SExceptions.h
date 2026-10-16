#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>

namespace Solstice
{
	struct SourceLocation
	{
		std::string source;
		std::size_t line = 0;
		std::size_t column = 0;

		bool IsValid() const noexcept
		{
			return line != 0;
		}
	};

	class SException : public std::runtime_error
	{
	public:
		explicit SException(std::string text);
		SException(SourceLocation location, std::string text);
		SException(int lineNumber, std::string text);

		const SourceLocation& Location() const noexcept;
		bool HasLocation() const noexcept;
		const std::string& Text() const noexcept;

		std::string message;

	private:
		static std::string Format(const SourceLocation& location, const std::string& text);

		SourceLocation mLocation;
		std::string mText;
	};
}