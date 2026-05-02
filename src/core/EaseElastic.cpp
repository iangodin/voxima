
#include "EaseElastic.h"

#include <cmath>

constexpr float pi = 3.14159265358979323846f;

Animation ease_in_elastic( float &value, float start, float end, int64_t total_frames )
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame <= total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames );

		float ease_t = 0.0f;
		if ( t == 0.0f )
		{
			ease_t = 0.0f;
		}
		else if ( t == 1.0f )
		{
			ease_t = 1.0f;
		}
		else
		{
			ease_t = -std::pow( 2.0f, 10.0f * t - 10.0f ) * std::sin( ( t * 10.0f - 10.75f ) * ( 2.0f * pi ) / 3.0f );
		}

		value = start + ( end - start ) * ease_t;

		if ( frame < total_frames )
		{
			co_yield {};
		}
	}
}

Animation ease_out_elastic( float &value, float start, float end, int64_t total_frames )
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame <= total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames );

		float ease_t = 0.0f;
		if ( t == 0.0f )
		{
			ease_t = 0.0f;
		}
		else if ( t == 1.0f )
		{
			ease_t = 1.0f;
		}
		else
		{
			ease_t = std::pow( 2.0f, -10.0f * t ) * std::sin( ( t * 10.0f - 0.75f ) * ( 2.0f * pi ) / 3.0f ) + 1.0f;
		}

		value = start + ( end - start ) * ease_t;

		if ( frame < total_frames )
		{
			co_yield {};
		}
	}
}

Animation ease_in_out_elastic( float &value, float start, float end, int64_t total_frames )
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame <= total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames );

		float ease_t = 0.0f;
		if ( t == 0.0f )
		{
			ease_t = 0.0f;
		}
		else if ( t == 1.0f )
		{
			ease_t = 1.0f;
		}
		else if ( t < 0.5f )
		{
			ease_t = -( std::pow( 2.0f, 20.0f * t - 10.0f ) * std::sin( ( 20.0f * t - 11.125f ) * ( 2.0f * pi ) / 4.5f ) ) / 2.0f;
		}
		else
		{
			ease_t = ( std::pow( 2.0f, -20.0f * t + 10.0f ) * std::sin( ( 20.0f * t - 11.125f ) * ( 2.0f * pi ) / 4.5f ) ) / 2.0f + 1.0f;
		}

		value = start + ( end - start ) * ease_t;

		if ( frame < total_frames )
		{
			co_yield {};
		}
	}
}

