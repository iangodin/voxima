
#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"

struct Circle : public Visual
{
	float radius = 1.F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Circle &operator*=( float scale ) override
	{
		radius *= scale;
		position *= scale;
		return *this;
	}
};
