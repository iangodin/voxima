#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>

// A nucleus with an optional superscript and/or subscript attached to its right
// (TeX rule 18, using the font's MATH constants). The scripts are expected to be built
// at script size by the caller (see script_style()). The nucleus baseline is at y = 0
// and the left edge at x = 0.
class Scripts : public Group
{
public:
	Scripts(
		const MathFont &font,
		float glyph_scale,
		std::unique_ptr<Visual> &&nucleus,
		std::unique_ptr<Visual> &&superscript,
		std::unique_ptr<Visual> &&subscript );
};
