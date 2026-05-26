
#include "EaseCubic.h"

#include <cmath>

Animation ease_cubic_in( float start, float end, int64_t total_frames )
{
	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );
		float ease_t = t * t * t;

		co_yield start + ( end - start ) * ease_t;
	}
}

Animation ease_cubic_out( float start, float end, int64_t total_frames )
{
	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );
		float ease_t = 1.0f - std::pow( 1.0f - t, 3.0f );

		co_yield start + ( end - start ) * ease_t;
	}
}

Animation ease_cubic_in_out( float start, float end, int64_t total_frames )
{
	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );

		float ease_t = t < 0.5f
			? 4.0f * t * t * t
			: 1.0f - std::pow( -2.0f * t + 2.0f, 3.0f ) / 2.0f;

		co_yield start + ( end - start ) * ease_t;
	}
}

