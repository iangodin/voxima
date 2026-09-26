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

Contour contour_of( const Path &path )
{
	Contour contour;
	contour.closed = false;
	Point current{ 0.F, 0.F };
	bool started = false;
	for ( const PathCommand &command: path.commands )
	{
		if ( const auto *move = std::get_if<MoveTo>( &command ) )
		{
			if ( started )
			{
				break; // (the first sub-path only)
			}
			contour.start = current = move->to;
			started = true;
		}
		else if ( const auto *line = std::get_if<LineTo>( &command ) )
		{
			contour.segs.push_back( { lerp( current, line->to, 1.F / 3.F ), lerp( current, line->to, 2.F / 3.F ), line->to } );
			current = line->to;
		}
		else if ( const auto *quad = std::get_if<QuadTo>( &command ) )
		{
			contour.segs.push_back( { lerp( current, quad->ctrl1, 2.F / 3.F ), lerp( quad->to, quad->ctrl1, 2.F / 3.F ), quad->to } );
			current = quad->to;
		}
		else if ( const auto *cubic = std::get_if<CubicTo>( &command ) )
		{
			contour.segs.push_back( { cubic->ctrl1, cubic->ctrl2, cubic->to } );
			current = cubic->to;
		}
		else if ( std::holds_alternative<Close>( command ) )
		{
			if ( distance( current, contour.start ) > 1e-6F )
			{
				contour.segs.push_back( { lerp( current, contour.start, 1.F / 3.F ), lerp( current, contour.start, 2.F / 3.F ), contour.start } );
			}
			contour.closed = true;
			break;
		}
	}
	return contour;
}

Point point_along( const Contour &contour, float fraction, Point *heading )
{
	fraction = std::clamp( fraction, 0.F, 1.F );
	const float target = contour_length( contour ) * fraction;
	float done = 0.F;
	Point from = contour.start;
	for ( size_t i = 0; i < contour.segs.size(); ++i )
	{
		const CubicSeg &seg = contour.segs[i];
		const float length = segment_length( from, seg );
		if ( done + length >= target || i + 1 == contour.segs.size() )
		{
			const float u = length > 0.F ? std::clamp( ( target - done ) / length, 0.F, 1.F ) : 0.F;
			const float v = 1.F - u;
			if ( heading )
			{
				// The derivative of the cubic
				Point d = ( seg.c1 - from ) * ( 3.F * v * v ) + ( seg.c2 - seg.c1 ) * ( 6.F * v * u ) + ( seg.to - seg.c2 ) * ( 3.F * u * u );
				float size = std::sqrt( d.lengthSquared() );
				if ( size < 1e-6F )
				{
					d = seg.to - from; // (a stopped point: the chord)
					size = std::sqrt( d.lengthSquared() );
				}
				*heading = size > 1e-6F ? d / size : Point{ 1.F, 0.F };
			}
			return from * ( v * v * v ) + seg.c1 * ( 3.F * v * v * u ) + seg.c2 * ( 3.F * v * u * u ) + seg.to * ( u * u * u );
		}
		done += length;
		from = seg.to;
	}
	if ( heading )
	{
		*heading = { 1.F, 0.F };
	}
	return contour.start;
}
