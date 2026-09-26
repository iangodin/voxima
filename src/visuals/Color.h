
#pragma once

struct Color
{
	float r = 1.F;
	float g = 1.F;
	float b = 1.F;
	float a = 1.F;

	Color() = default;
	Color( float rr, float gg, float bb, float aa = 1.F ) :
		r( rr ), g( gg ), b( bb ), a( aa )
	{
	}

	// From 0 to 255 values, as colors are usually written (rgb( 97, 175, 239 ) or #61AFEF)
	static Color rgb( int red, int green, int blue )
	{
		return { static_cast<float>( red ) / 255.F, static_cast<float>( green ) / 255.F, static_cast<float>( blue ) / 255.F };
	}

	Color operator*( const Color &other ) const
	{
		return { r * other.r, g * other.g, b * other.b, a * other.a };
	}

	bool operator==( const Color &other ) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}
};
