
#include "EaseElastic.h"

#include <cmath>
#include <iostream>

constexpr float pi = 3.14159265358979323846f;

Animation EaseElasticIn::operator()( float &value, float start, float end, int64_t total_frames ) const
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	const float s = amplitude < 1.F ?  period / 4.F : period / ( 2.F * pi ) * std::asin( 1.F / amplitude );

	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );

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
			ease_t = -amplitude * std::pow( 2.0f, 10.0f * t - 10.0f ) * std::sin( ( t * 10.0f - 10.f - s ) * ( 2.0f * pi ) / period );
		}

		value = start + ( end - start ) * ease_t;

		co_yield {};
	}
	value = end;
}

Animation EaseElasticOut::operator()( float &value, float start, float end, int64_t total_frames ) const
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	const float s = amplitude < 1.F ?  period / 4.F : period / ( 2.F * pi ) * std::asin( 1.F / amplitude );

	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );

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
			ease_t = amplitude * std::pow( 2.0f, -10.0f * t ) * std::sin( ( t * 10.0f - s ) * ( 2.0f * pi ) / period ) + 1.0f;
		}

		value = start + ( end - start ) * ease_t;

		co_yield {};
	}
	value = end;
}

Animation EaseElasticInOut::operator()( float &value, float start, float end, int64_t total_frames ) const
{
	if ( total_frames <= 0 )
	{
		value = end;
		co_return;
	}

	const float s = amplitude < 1.F ?  period / 4.F : period / ( 2.F * pi ) * std::asin( 1.F / amplitude );

	for ( int64_t frame = 0; frame < total_frames; ++frame )
	{
		float t = static_cast<float>( frame ) / static_cast<float>( total_frames - 1 );

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
			ease_t = -( amplitude * std::pow( 2.0f, 20.0f * t - 10.0f ) * std::sin( ( 20.0f * t - 10.f - s ) * ( 2.0f * pi ) / period ) ) / 2.0f;
		}
		else
		{
			ease_t = ( amplitude * std::pow( 2.0f, -20.0f * t + 10.0f ) * std::sin( ( 20.0f * t - 10.f - s ) * ( 2.0f * pi ) / period ) ) / 2.0f + 1.0f;
		}

		value = start + ( end - start ) * ease_t;

		co_yield {};
	}
	value = end;
}

