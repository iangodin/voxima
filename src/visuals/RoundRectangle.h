
#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"

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

	RoundRectangle &operator*=( float scale ) override
	{
		half_width *= scale;
		half_height *= scale;
		scale_common( scale );
		top_left_radius *= scale;
		top_right_radius *= scale;
		bottom_left_radius *= scale;
		bottom_right_radius *= scale;
		return *this;
	}
};
