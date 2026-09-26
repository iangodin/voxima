#include "EaseCubic.h"

float ease_cubic_in( float t )
{
	return t * t * t;
}

float ease_cubic_out( float t )
{
	return 1.0f - std::pow( 1.0f - t, 3.0f );
}

float ease_cubic_in_out( float t )
{
	return t < 0.5f
		? 4.0f * t * t * t
		: 1.0f - std::pow( -2.0f * t + 2.0f, 3.0f ) / 2.0f;
}
