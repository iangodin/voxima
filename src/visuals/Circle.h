
#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"

struct Circle : public Visual
{
	Circle() = default;

	Circle( float r, const Color &c = {} )
		: radius( r )
	{
		color = c;
	}

	float radius = 1.F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Circle &operator*=( float scale ) override
	{
		radius *= scale;
		scale_common( scale );
		return *this;
	}
};
