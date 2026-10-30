
#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"
#include "visuals/Path.h"

#include <cstdint>
#include <optional>

struct FontFace;

struct Glyph : public Visual
{
	const FontFace *font_face = nullptr;
	uint32_t index = 0;
	std::vector<PathCommand> commands;

	float bearingX = 0.F;
	float bearingY = 0.F;
	float advanceX = 0.F;
	float advanceY = 0.F;

	// From the font's MATH table when it has one (0 otherwise): how far a superscript
	// is pushed right of this glyph. Scales with the glyph.
	float italicCorrection = 0.F;

	// From the MATH table when the font has one: the x, from the glyph's origin, where an
	// accent above it should be centered. Unset when the font doesn't say.
	std::optional<float> topAccentAttachment;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Glyph &operator*=( float scale ) override
	{
		scale_common( scale );
		bearingX *= scale;
		bearingY *= scale;
		advanceX *= scale;
		advanceY *= scale;
		italicCorrection *= scale;
		if ( topAccentAttachment )
		{
			*topAccentAttachment *= scale;
		}
		for ( auto &cmd : commands )
		{
			std::visit( [scale](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, MoveTo>) arg.to *= scale;
				else if constexpr (std::is_same_v<T, LineTo>) arg.to *= scale;
				else if constexpr (std::is_same_v<T, QuadTo>) { arg.to *= scale; arg.ctrl1 *= scale; }
				else if constexpr (std::is_same_v<T, CubicTo>) { arg.to *= scale; arg.ctrl1 *= scale; arg.ctrl2 *= scale; }
			}, cmd);
		}
		return *this;
	}
};
