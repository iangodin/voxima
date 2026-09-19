#pragma once

#include "visuals/Group.h"
#include "visuals/Row.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>

// Content between delimiters that stretch to fit it (TeX's \left ... \right, rule 19).
// `open` and `close` are code points such as ( ) [ ] { } | or U+27E8; pass 0 for none.
// The delimiters are centered on the math axis and are the smallest that cover the content
// (TeX: at least 90.1% of twice its reach from the axis, and at most half an em short of it).
// A Row underneath, so it is an inner atom when placed in another Row.
class Delimited : public Row
{
public:
	Delimited(
		const MathFont &font,
		float glyph_scale,
		char32_t open,
		std::unique_ptr<Visual> &&content,
		char32_t close,
		MathStyle style = MathStyle::display() );
};

// n over k in parentheses, without a rule
std::unique_ptr<Group> make_binomial(
	const MathFont &font,
	float glyph_scale,
	std::unique_ptr<Visual> &&top,
	std::unique_ptr<Visual> &&bottom,
	MathStyle style = MathStyle::display() );
