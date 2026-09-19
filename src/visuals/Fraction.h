#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>

struct FractionOptions
{
	bool rule = true;       // false: a stack (TeX's \atop), as inside a binomial
	bool side_space = true; // TeX's null-delimiter space; not wanted when real delimiters surround it
};

// A fraction: numerator over denominator with a rule centered on the math axis.
// Children are placed relative to their own `baseline` anchors, and the fraction's
// baseline is at y = 0 with its left edge at x = 0.
class Fraction : public Group
{
public:
	Fraction(
		const MathFont &font,
		float glyph_scale,
		std::unique_ptr<Visual> &&numerator,
		std::unique_ptr<Visual> &&denominator,
		FractionOptions options = {} );
};
