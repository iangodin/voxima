#pragma once

#include "Highlight.h"

#include <fonts/FontStyle.h>
#include <visuals/Group.h>

#include <memory>
#include <string>
#include <string_view>

class Theme;

struct CodeLayout
{
	// The font: the theme's code (monospaced) font, at whatever size this says
	FontStyle style = FontStyle{ Intent::CODE };

	// Columns to a tab stop: tabs become spaces (a font has nothing to draw for a tab)
	size_t tab_width = 4;

	// Baseline to baseline, as a multiple of the font size
	float line_spacing = 1.2F;
};

// Source code, highlighted in the theme's code palette (Theme::code_palette) and laid out in its code font, a line of
// the source to a line of text, spaces kept so the columns line up. The language comes first, where it can be seen:
//
//     auto listing = make_code( *stage.theme, code::Language::CPP, R"cpp(
//     int main()
//     {
//         return 0;
//     }
//     )cpp" );
//
// As make_styled_text: the origin is the start of the first line's baseline, and each line is a TextLine, the
// group's children in order (line i of the source is child i). A newline straight after the opening quote and
// one before the closing quote are dropped, as in the example.
std::unique_ptr<Group> make_code( Theme &theme, code::Language language, std::string_view source, const CodeLayout &layout = {} );

// The source as it is laid out: tabs as spaces to the next tab stop, no carriage returns, and without a newline at the
// very start or the very end
std::string prepare_code( std::string_view source, size_t tab_width );
