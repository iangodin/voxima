#pragma once

#include "visuals/Glyph.h"

struct MathGlyph : public Glyph
{
	float italicCorrection = 0.F;
	float topAccentAttachment = 0.F;

	MathGlyph &operator*=( float scale ) override
	{
		Glyph::operator*=( scale );
		italicCorrection *= scale;
		topAccentAttachment *= scale;
		return *this;
	}
};
