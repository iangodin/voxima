#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

struct Ellipse : public Visual
{
	float half_width = 1.0F;
	float half_height = 0.5F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}
};
