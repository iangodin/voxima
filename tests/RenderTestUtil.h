#pragma once

#include <exporters/SVGWriter.h>
#include <visuals/Group.h>
#include <visuals/Visual.h>

#include <lunasvg/lunasvg.h>

#include <cstdint>
#include <cstdlib>
#include <sstream>
#include <string>

// What the tests of drawing look at: a visual as the stage writes it, at 100 pixels to a unit, from -8 to 8 and -4.5 to 4.5
namespace render_test
{
inline std::string svg_of( const Visual &visual, float flatness = 0.F )
{
	std::ostringstream body;
	SVGWriter writer( body, 0, flatness );
	visual.accept( writer );
	std::ostringstream out;
	out << "<svg width=\"1600\" height=\"900\" viewBox=\"-8 -4.5 16 9\" xmlns=\"http://www.w3.org/2000/svg\">";
	writer.write_defs( out );
	out << "<rect x=\"-8\" y=\"-4.5\" width=\"16\" height=\"9\" fill=\"rgb(0,0,0)\"/><g transform=\"scale(1, -1)\">" << body.str() << "</g></svg>";
	return out.str();
}

inline lunasvg::Bitmap render( const Visual &visual, float flatness = 0.F )
{
	auto document = lunasvg::Document::loadFromData( svg_of( visual, flatness ) );
	return document ? document->renderToBitmap() : lunasvg::Bitmap{};
}

struct Pixel
{
	int r, g, b;
};

// The pixel at a point of the stage (x right, y up)
inline Pixel pixel_at( const lunasvg::Bitmap &bitmap, float x, float y )
{
	const int column = static_cast<int>( ( x + 8.F ) * 100.F );
	const int row = static_cast<int>( ( 4.5F - y ) * 100.F );
	const std::uint8_t *p = bitmap.data() + row * bitmap.stride() + column * 4;
	return { p[2], p[1], p[0] };
}

// How many pixels of two pictures clearly differ
inline int pixels_apart( const lunasvg::Bitmap &a, const lunasvg::Bitmap &b, int by = 96 )
{
	int count = 0;
	for ( int row = 0; row < a.height(); ++row )
	{
		for ( int column = 0; column < a.width(); ++column )
		{
			const std::uint8_t *p = a.data() + row * a.stride() + column * 4;
			const std::uint8_t *q = b.data() + row * b.stride() + column * 4;
			if ( std::abs( p[0] - q[0] ) > by || std::abs( p[1] - q[1] ) > by || std::abs( p[2] - q[2] ) > by )
			{
				++count;
			}
		}
	}
	return count;
}
}
