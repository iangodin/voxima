#include "visuals/Trim.h"

#include <algorithm>
#include <cmath>

namespace
{

float distance( Point a, Point b )
{
	return std::sqrt( ( b - a ).lengthSquared() );
}

Point lerp( Point a, Point b, float t )
{
	return a + ( b - a ) * t;
}

// Between the straight line and the control polygon: the curve is longer than the first, shorter than the second
float segment_length( Point from, const CubicSeg &seg )
{
	const float chord = distance( from, seg.to );
	const float polygon = distance( from, seg.c1 ) + distance( seg.c1, seg.c2 ) + distance( seg.c2, seg.to );
	return ( 2.F * chord + polygon ) / 3.F;
}

}

float contour_length( const Contour &contour )
{
	float length = 0.F;
	Point from = contour.start;
	for ( const CubicSeg &seg: contour.segs )
	{
		length += segment_length( from, seg );
		from = seg.to;
	}
	return length;
}

std::vector<PathCommand> trimmed_commands( const Contour &contour, float fraction )
{
	std::vector<PathCommand> commands;
	commands.push_back( MoveTo{ contour.start } );
	fraction = std::clamp( fraction, 0.F, 1.F );
	if ( fraction <= 0.F )
	{
		return commands;
	}

	const float target = contour_length( contour ) * fraction;
	float done = 0.F;
	Point from = contour.start;
	for ( const CubicSeg &seg: contour.segs )
	{
		const float length = segment_length( from, seg );
		if ( fraction < 1.F && done + length > target )
		{
			// Only part of this one: the curve up to u (de Casteljau)
			const float u = length > 0.F ? ( target - done ) / length : 0.F;
			const Point a = lerp( from, seg.c1, u );
			const Point b = lerp( seg.c1, seg.c2, u );
			const Point c = lerp( seg.c2, seg.to, u );
			const Point ab = lerp( a, b, u );
			const Point bc = lerp( b, c, u );
			commands.push_back( CubicTo{ lerp( ab, bc, u ), a, ab } );
			return commands;
		}
		commands.push_back( CubicTo{ seg.to, seg.c1, seg.c2 } );
		done += length;
		from = seg.to;
	}
	if ( contour.closed )
	{
		commands.push_back( Close{} );
	}
	return commands;
}
