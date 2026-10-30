#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"
#include <string>

struct Image : public Visual
{
	std::string path;
	float half_width = 1.0F;
	float half_height = 1.0F;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Image &operator*=( float scale ) override
	{
		half_width *= scale;
		half_height *= scale;
		scale_common( scale );
		return *this;
	}
};
