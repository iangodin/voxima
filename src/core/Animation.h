
#pragma once

#include "core/Director.h"
#include <cmath>

inline Director ease_in_cubic( float &value, float start, float end, int64_t total_frames )
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame <= total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames );
		float ease_t = t * t * t;

		value = start + ( end - start ) * ease_t;

		if ( frame < total_frames )
		{
			co_yield {};
		}
	}
}

inline Director ease_out_cubic( float &value, float start, float end, int64_t total_frames )
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame <= total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames );
		float ease_t = 1.0f - std::pow( 1.0f - t, 3.0f );

		value = start + ( end - start ) * ease_t;

		if ( frame < total_frames )
		{
			co_yield {};
		}
	}
}

inline Director ease_in_out_cubic( float &value, float start, float end, int64_t total_frames )
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame <= total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames );
		
		float ease_t = t < 0.5f 
			? 4.0f * t * t * t 
			: 1.0f - std::pow( -2.0f * t + 2.0f, 3.0f ) / 2.0f;

		value = start + ( end - start ) * ease_t;

		if ( frame < total_frames )
		{
			co_yield {};
		}
	}
}
