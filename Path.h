
#pragma once

#include "Visual.h"
#include "VisualVisitor.h"
#include "Point.h"
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
};

