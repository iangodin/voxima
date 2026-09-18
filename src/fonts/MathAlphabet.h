#pragma once

#include "fonts/FontStyle.h"

#include <string>
#include <string_view>

// Styles of the Unicode "Mathematical Alphanumeric Symbols" block. Math fonts carry
// these as ordinary glyphs, so italic, bold, script etc. letters in a formula are code
// points of their own rather than a separate font file.
enum class MathAlphabet
{
	UPRIGHT, // unchanged (function names like sin, units, digits)
	ITALIC,  // Latin letters and lowercase Greek; digits and capital Greek stay upright
	BOLD,
	BOLD_ITALIC,
	SCRIPT,
	FRAKTUR,
	DOUBLE_STRUCK,
	SANS,
	MONO,
};

// The code point for `c` in the given variant, or `c` itself if the variant has no form
// of it. Handles the letters Unicode left in older blocks (ℎ, ℬ, ℭ, ℝ ...).
char32_t map_math_char( char32_t c, MathAlphabet variant );

// The same for a whole UTF-8 string.
std::string map_math_text( std::string_view utf8, MathAlphabet variant );

// The alphabet a style asks for, or UPRIGHT when no mapping is needed.
//  - MATH_ITALIC slant (the default for the math intent) -> ITALIC, or BOLD_ITALIC if bold
//  - .upright() -> no mapping
//  - in the math intent, with no slant given, an explicit .bold() / .sans() / .mono() /
//    .script() picks that alphabet upright (like \mathbf, \mathsf, ...)
MathAlphabet math_alphabet( const FontStyle &style );
