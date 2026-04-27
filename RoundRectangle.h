
#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

struct RoundRectangle : public Visual
{
	float half_width = 0.F;
	float half_height = 0.F;

	float top_left_radius = 0.F;
	float top_right_radius = 0.F;
	float bottom_left_radius = 0.F;
	float bottom_right_radius = 0.F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}
};
