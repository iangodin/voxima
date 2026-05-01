
#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"
#include "visuals/Point.h"
#include <vector>

struct PathCommand
{
	enum Type { MoveTo, LineTo, QuadTo, CubicTo } type;
	Point to;
	Point ctrl1;
	Point ctrl2;
};

struct Path : public Visual
{
	std::vector<PathCommand> commands;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Path &operator*=( float scale ) override
	{
		for ( auto &command: commands )
		{
			command.to *= scale;
			command.ctrl1 *= scale;
			command.ctrl2 *= scale;
		}
		position *= scale;
		return *this;
	}
};

