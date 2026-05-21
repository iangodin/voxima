
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

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Glyph &operator*=( float scale ) override
	{
		position *= scale;
		for ( auto &cmd : commands )
		{
			cmd.to *= scale;
			cmd.ctrl1 *= scale;
			cmd.ctrl2 *= scale;
		}
		return *this;
	}
};
