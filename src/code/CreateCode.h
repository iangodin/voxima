#pragma once

#include "Highlight.h"

#include <fonts/FontStyle.h>
#include <visuals/Group.h>
#include <visuals/TextLine.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

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

struct Glyph;

// A glyph of a listing, and which character of the source it is
struct CodeGlyph
{
	size_t offset = 0; // byte offset in CodeListing::text
	size_t line = 0;   // counting from 0
	size_t column = 0; // in characters, counting from 0
	Glyph *glyph = nullptr;
};

// Laid-out source code (make_code): a Group of TextLines, one to a line of the source, that also knows which glyph is
// which character, for animating it a character at a time (see Typewriter)
//
// The first child is a background layer (empty until LineHighlight draws bands in it), behind the text; the lines
// come after it: use line( i ) and line_count(), not child numbers.
class CodeListing : public Group
{
public:
	// The source as it was laid out (see prepare_code)
	std::string text;

	// Behind the text: what LineHighlight draws. It moves with the listing.
	Group *background = nullptr;

	// The lines of the source, counting from 0 (an empty line is there too, with nothing on it)
	size_t line_count() const;
	TextLine &line( size_t index ) const;

	// How the lines sit, in the listing's own coordinates (before anything moves it): the size of the font, how far
	// apart the lines are, and the left and right of the text
	float em = 1.F;
	float line_pitch = 1.F;
	float left = 0.F;
	float right = 0.F;

	// Every glyph, spaces too, in the order of the source. They belong to the listing: the pointers stay good for as
	// long as it does.
	std::vector<CodeGlyph> glyphs;
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
// group's children in order (line i of the source is child i). Put it on the stage and keep the reference to it:
//
//     auto &listing = stage.visuals.add( make_code( ... ) ); A newline straight after the opening quote and
// one before the closing quote are dropped, as in the example.
std::unique_ptr<CodeListing> make_code( Theme &theme, code::Language language, std::string_view source, const CodeLayout &layout = {} );

// The source as it is laid out: tabs as spaces to the next tab stop, no carriage returns, and without a newline at the
// very start or the very end
std::string prepare_code( std::string_view source, size_t tab_width );
