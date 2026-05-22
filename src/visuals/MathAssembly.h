#pragma once

#include "visuals/Group.h"

class MathAssembly : public Group
{
public:
	float italicCorrection = 0.F;

	MathAssembly &operator*=( float scale ) override
	{
		Group::operator*=( scale );
		italicCorrection *= scale;
		return *this;
	}
};
