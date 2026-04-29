
#pragma once

#include "Visual.h"
#include "VisualVisitor.h"
#include "Path.h"
#include <mfl/font_family.hpp>

struct Glyph : public Visual
{
	mfl::font_family family;
	unsigned int index;
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
