
#include "EaseLinear.h"

#include <cmath>

Animation ease_linear( float &value, float start, float end, int64_t total_frames )
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );
		value = start + ( end - start ) * t;

		co_yield NextFrame{};
	}
}

