#include "EaseBack.h"

float EaseBackIn::operator()( float t ) const
{
	return t * t * ( ( overshoot + 1.0f ) * t - overshoot );
}

float EaseBackOut::operator()( float t ) const
{
	t -= 1.0f;
	return t * t * ( ( overshoot + 1.0f ) * t + overshoot ) + 1.0f;
}

float EaseBackInOut::operator()( float t ) const
{
	constexpr float m = 1.525f;

	t *= 2.0f;
	if ( t < 1.0f )
	{
		return 0.5f * ( t * t * ( ( overshoot * m + 1.0f ) * t - overshoot * m ) );
	}

	t -= 2.0f;
	return 0.5f * ( t * t * ( ( overshoot * m + 1.0f ) * t + overshoot * m ) + 2.0f );
}
