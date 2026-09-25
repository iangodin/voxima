#pragma once

#include "visuals/Glyph.h"

#include <fonts/FontFace.h>
#include <fonts/FontStack.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

// A character that cannot be drawn properly is not left to fail quietly (a box in the text's own color, a tiny
// diamond, or a crash): it is drawn as a loud magenta box with the character's code inside, so it is found and
// fixed. That is done for
//  - a character no font in the stack has, or that only the catch-all NotDef font at the end of the stack has
//    (which draws a plain box for anything), and
//  - U+FFFD, the replacement character: what a missing script key or a badly encoded string turns into,
//    which a font would draw as an easily missed little diamond.
// Every one is also recorded, and the producer prints them when the render is done.

inline constexpr char32_t REPLACEMENT_CHARACTER = 0xFFFD;

// Tag on the box a missing glyph is drawn as (see Tags.h)
inline constexpr const char *MISSING_GLYPH_TAG = "missing-glyph";

// Whether the character is drawn as a marker: no real font has it, or it is the replacement character
bool needs_marker( const FontStack &stack, char32_t codepoint );

// The marker for a character, in the units of the font (like create_glyph, so it is laid out, scaled and
// moved the same way): a magenta frame the height of a letter with the code (FFFD, 1F600 ...) in small
// type inside, or an empty frame if the font lacks the digits.
std::unique_ptr<Glyph> create_missing_glyph( char32_t codepoint, const FontFace &font );

// ---- The record -------------------------------------------------------------------------------------

struct MissingGlyphReport
{
	char32_t codepoint = 0;
	std::string context; // the text it was in
	size_t count = 0;    // how many times
};

// Note one occurrence. Repeats of the same character in the same text are counted, not listed again.
void report_missing_glyph( char32_t codepoint, std::string_view context );

const std::vector<MissingGlyphReport> &missing_glyph_reports();
void clear_missing_glyph_reports();

// A warning listing them (empty if there are none), for the end of a render
std::string describe_missing_glyphs();
