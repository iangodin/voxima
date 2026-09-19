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
