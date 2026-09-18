
#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"
#include "visuals/Path.h"

#include <cstdint>

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

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Glyph &operator*=( float scale ) override
	{
		position *= scale;
		baseline *= scale;
		math_axis *= scale;
		bearingX *= scale;
		bearingY *= scale;
		advanceX *= scale;
		advanceY *= scale;
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
