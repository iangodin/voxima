
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

	// From a hex value: Color::hex( 0x61AFEF )
	static Color hex( unsigned int rgb )
	{
		return Color::rgb( static_cast<int>( ( rgb >> 16 ) & 0xFF ), static_cast<int>( ( rgb >> 8 ) & 0xFF ), static_cast<int>( rgb & 0xFF ) );
	}

	// Pure white, which is what a visual is until it is given a color, stands for the theme's text color (Stage::foreground):
	// what is white is drawn in that, keeping its alpha, and anything else as it is. So text and lines are dark on a light
	// background and light on a dark one, while everything given a color (a tint, an effect, a palette's accent)
	// keeps it. With a white foreground (the default) this changes nothing.
	Color resolved( const Color &foreground ) const
	{
		return r == 1.F && g == 1.F && b == 1.F ? Color{ foreground.r, foreground.g, foreground.b, a * foreground.a } : *this;
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
