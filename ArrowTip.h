#pragma once

#include "Visual.h"
#include "VisualVisitor.h"
#include "Point.h"
#include <cmath>

struct ArrowTip : public Visual
{
	Point start = { 0.F, 0.F };
	Point end = { 0.F, 0.F };
	float size = 0.5F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	ArrowTip &operator*=( float scale ) override
	{
		start *= scale;
		end *= scale;
		size *= scale;
		position *= scale;
		return *this;
	}
};
