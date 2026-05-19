
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

	Color operator*( const Color &other ) const
	{
		return { r * other.r, g * other.g, b * other.b, a * other.a };
	}

	bool operator==( const Color &other ) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}
};
