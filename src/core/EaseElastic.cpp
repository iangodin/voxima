#include "EaseElastic.h"

#include <cmath>

namespace
{
constexpr float pi = 3.14159265358979323846f;

float shift( float amplitude, float period )
{
	return amplitude < 1.F ? period / 4.F : period / ( 2.F * pi ) * std::asin( 1.F / amplitude );
}
} // namespace

float EaseElasticIn::operator()( float t ) const
{
	const float s = shift( amplitude, period );

	if ( t == 0.0f )
	{
		return 0.0f;
	}
	if ( t == 1.0f )
	{
		return 1.0f;
	}
	return -amplitude * std::pow( 2.0f, 10.0f * t - 10.0f ) * std::sin( ( t * 10.0f - 10.f - s ) * ( 2.0f * pi ) / period );
}

float EaseElasticOut::operator()( float t ) const
{
	const float s = shift( amplitude, period );

	if ( t == 0.0f )
	{
		return 0.0f;
	}
	if ( t == 1.0f )
	{
		return 1.0f;
	}
	return amplitude * std::pow( 2.0f, -10.0f * t ) * std::sin( ( t * 10.0f - s ) * ( 2.0f * pi ) / period ) + 1.0f;
}

float EaseElasticInOut::operator()( float t ) const
{
	const float s = shift( amplitude, period );

	if ( t == 0.0f )
	{
		return 0.0f;
	}
	if ( t == 1.0f )
	{
		return 1.0f;
	}
	if ( t < 0.5f )
	{
		return -( amplitude * std::pow( 2.0f, 20.0f * t - 10.0f ) * std::sin( ( 20.0f * t - 10.f - s ) * ( 2.0f * pi ) / period ) ) / 2.0f;
	}
	return ( amplitude * std::pow( 2.0f, -20.0f * t + 10.0f ) * std::sin( ( 20.0f * t - 10.f - s ) * ( 2.0f * pi ) / period ) ) / 2.0f + 1.0f;
}
