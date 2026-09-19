#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>

// A nucleus (normally a large operator such as a sum) with an optional limit above and
// below, all centered on one another (TeX rule 13a, using the font's UpperLimit and
// LowerLimit constants). The limits are expected at script size. A slanted operator
// pushes the upper limit right and the lower limit left by half its italic correction.
// The nucleus baseline is at y = 0 and the left edge at x = 0.
class Limits : public Group
{
public:
	Limits(
		const MathFont &font,
		float glyph_scale,
		std::unique_ptr<Visual> &&nucleus,
		std::unique_ptr<Visual> &&upper,
		std::unique_ptr<Visual> &&lower );
};
