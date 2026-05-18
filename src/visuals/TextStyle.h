
#include "Color.h"

#include <fonts/FontStyle.h>

#include <optional>
#include <string>

struct TextStyle
{
	FontStyle font_style = {};

	std::optional<std::string> language;
	std::optional<std::string> script;
	std::optional<size_t> bidi_level;
	std::optional<size_t> line;
	std::optional<Color> color;

	bool operator==( const TextStyle &other ) const
	{
		return font_style == other.font_style 
			   && language == other.language 
			   && script == other.script 
			   && bidi_level == other.bidi_level 
			   && line == other.line 
			   && color == other.color;
	}
};

