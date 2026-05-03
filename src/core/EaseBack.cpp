#include "EaseBack.h"

#include <cmath>

Animation EaseBackIn::operator()( float &value, float start, float end, int64_t total_frames ) const
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );
		float ease_t = t * t * ( ( overshoot + 1.0f ) * t - overshoot );

		value = start + ( end - start ) * ease_t;

		co_yield {};
	}
	value = end;
}

Animation EaseBackOut::operator()( float &value, float start, float end, int64_t total_frames ) const
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );
		t -= 1.0f;
		float ease_t = t * t * ( ( overshoot + 1.0f ) * t + overshoot ) + 1.0f;

		value = start + ( end - start ) * ease_t;

		co_yield {};
	}
	value = end;
}

Animation EaseBackInOut::operator()( float &value, float start, float end, int64_t total_frames ) const
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	constexpr float m = 1.525f;

	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );
		t *= 2.0f;

		float ease_t;
		if ( t < 1.0f )
		{
			ease_t = 0.5f * ( t * t * ( ( overshoot * m + 1.0f ) * t - overshoot * m ) );
		}
		else
		{
			t -= 2.0f;
			ease_t = 0.5f * ( t * t * ( ( overshoot * m + 1.0f ) * t + overshoot * m ) + 2.0f );
		}

		value = start + ( end - start ) * ease_t;

		co_yield {};
	}
	value = end;
}
