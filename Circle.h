
#pragma once

#include "Visual.h"
#include "VisualVisitor.h"

struct Circle : public Visual
{
	float radius = 1.F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}
};
