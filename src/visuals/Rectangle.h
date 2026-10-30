
#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

struct Rectangle : public Visual
{
	Rectangle() = default;

	Rectangle( float half_w, float half_h, const Color &c = {} )
		: half_width( half_w ), half_height( half_h )
	{
		color = c;
	}

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
