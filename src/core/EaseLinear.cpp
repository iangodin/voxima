
#include "EaseLinear.h"

#include <cmath>

Animation ease_linear( float start, float end, int64_t total_frames )
{
	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );
		co_yield start + ( end - start ) * t;
	}
}

