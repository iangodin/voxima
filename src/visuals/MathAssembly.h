#pragma once

#include "visuals/Group.h"
#include "visuals/VisualVisitor.h"

class MathAssembly : public Group
{
public:
	float italicCorrection = 0.F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	MathAssembly &operator*=( float scale ) override
	{
		Group::operator*=( scale );
		italicCorrection *= scale;
		return *this;
	}
};
