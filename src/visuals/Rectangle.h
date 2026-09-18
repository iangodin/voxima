
#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

struct Rectangle : public Visual
{
	float half_width = 0.F;
	float half_height = 0.F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Rectangle &operator*=( float scale ) override
	{
		half_width *= scale;
		half_height *= scale;
		scale_common( scale );
		return *this;
	}
};
