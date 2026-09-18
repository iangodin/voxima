#pragma once

#include "visuals/Glyph.h"

struct MathGlyph : public Glyph
{
	float topAccentAttachment = 0.F;

	MathGlyph &operator*=( float scale ) override
	{
		Glyph::operator*=( scale );
		topAccentAttachment *= scale;
		return *this;
	}
};
