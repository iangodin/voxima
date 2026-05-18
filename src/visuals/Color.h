
#pragma once

struct Color
{
	float r = 1.F;
	float g = 1.F;
	float b = 1.F;
	float a = 1.F;

	Color operator*( const Color &other ) const
	{
		return { r * other.r, g * other.g, b * other.b, a * other.a };
	}

	bool operator==( const Color &other ) const
	{
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}
};
