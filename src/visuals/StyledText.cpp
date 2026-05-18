
#include "StyledText.h"

std::optional<size_t> StyledText::index( size_t line, size_t column )
{
	if ( line < _lines.size() )
	{
		auto text = _lines.at( line );
		if ( column < text.size() )
		{
			return static_cast<size_t>( std::distance( _utf8.c_str(), text.data() + column ) );
		}
	}

	return std::nullopt;
}

void StyledText::split_lines( void )
{
	// Split the UTF-8 text into lines based on newline characters
	// Lines are stored as string_views pointing into _utf8

	size_t pos = 0;
	while ( pos < _utf8.size() )
	{
		// Find the next newline character
		size_t newline_pos = _utf8.find( '\n', pos );
		
		if ( newline_pos == std::string::npos )
		{
			// No more newline, add the rest of the text as one final line
			_lines.push_back( std::string_view( _utf8.data() + pos, _utf8.size() - pos ) );
			break;
		}
		else
		{
			// Add text before newline as a line
			_lines.push_back( std::string_view( _utf8.data() + pos, newline_pos - pos ) );
			
			// Move to after the newline
			pos = newline_pos + 1;
		}
	}
}
