#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>

// A rule above the content (TeX's \overline, rule 9): the font's OverbarVerticalGap above
// the content, OverbarRuleThickness thick, with OverbarExtraAscender of room above it.
// The content's baseline is at y = 0 and its left edge at x = 0; the rule is as wide as it.
class Overline : public Group
{
public:
	Overline( const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&content );
};

// The same below the content (\underline, rule 10), with the Underbar constants.
class Underline : public Group
{
public:
	Underline( const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&content );
};

// A brace that stretches to fit above the content (\overbrace), using the Overbar constants
// for its gap and the room left above it. Put a superscript on the whole thing (as TeX does)
// to label it, e.g. superscript( overbrace( content ), label ).
class Overbrace : public Group
{
public:
	Overbrace( const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&content );
};

// The same below the content (\underbrace), with the Underbar constants; label with a
// subscript on the whole thing.
class Underbrace : public Group
{
public:
	Underbrace( const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&content );
};
