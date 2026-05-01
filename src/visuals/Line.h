#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"
#include "visuals/Point.h"

struct Line : public Visual
{
	Point start = { 0.F, 0.F };
	Point end = { 0.F, 0.F };

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Line &operator*=( float scale ) override
	{
		start *= scale;
		end *= scale;
		position *= scale;
		return *this;
	}
};
