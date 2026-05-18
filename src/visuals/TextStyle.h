
#include "Color.h"

#include <fonts/FontStyle.h>

#include <optional>

struct TextStyle
{
	FontStyle font_style;

	std::optional<std::string> language;
	std::optional<std::string> script;
	std::optional<size_t> bidi_level;
	std::optional<size_t> line;
};

