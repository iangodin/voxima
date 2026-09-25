#include "visuals/Morph.h"

#include <algorithm>

namespace
{

float lerp( float a, float b, float t )
{
	return a + ( b - a ) * t;
}

Point lerp( Point a, Point b, float t )
{
	return a + ( b - a ) * t;
}

// Beyond [0, 1] positions keep extrapolating, but colors must stay valid
Color lerp( const Color &a, const Color &b, float t )
{
	auto channel = [&]( float x, float y )
	{
		return std::clamp( lerp( x, y, t ), 0.F, 1.F );
	};
	return { channel( a.r, b.r ), channel( a.g, b.g ), channel( a.b, b.b ), channel( a.a, b.a ) };
}

// The same outline, invisible
Color transparent( Color c )
{
	c.a = 0.F;
	return c;
}

}

Morph::Morph( const Visual &from, const Visual &to, const MorphOptions &options ) :
	_owned( std::make_unique<Group>() ),
	_root( _owned.get() )
{
	const auto shapes_a = to_outline( from );
	const auto shapes_b = to_outline( to );

	const size_t count = std::max( shapes_a.size(), shapes_b.size() );
	for ( size_t i = 0; i < count; ++i )
	{
		Part part;
		if ( i < shapes_a.size() && i < shapes_b.size() )
		{
			const auto &a = shapes_a[i];
			const auto &b = shapes_b[i];
			part.contours = match_contours( a, b, options.costs );
			part.fill_a = a.fill;
			part.fill_b = b.fill;
			part.stroke_a = a.stroke;
			part.stroke_b = b.stroke;
			part.thickness_a = a.stroke_thickness;
			part.thickness_b = b.stroke_thickness;
		}
		else if ( i < shapes_a.size() )
		{
			// Nothing to become: fades out where it is
			const auto &a = shapes_a[i];
			for ( const auto &c: a.contours )
			{
				part.contours.push_back( { c, c } );
			}
			part.fill_a = a.fill;
			part.fill_b = transparent( a.fill );
			part.stroke_a = a.stroke;
			part.stroke_b = transparent( a.stroke );
			part.thickness_a = part.thickness_b = a.stroke_thickness;
		}
		else
		{
			const auto &b = shapes_b[i];
			for ( const auto &c: b.contours )
			{
				part.contours.push_back( { c, c } );
			}
			part.fill_a = transparent( b.fill );
			part.fill_b = b.fill;
			part.stroke_a = transparent( b.stroke );
			part.stroke_b = b.stroke;
			part.thickness_a = part.thickness_b = b.stroke_thickness;
		}
		part.path = &_root->add( std::make_unique<Path>() );
		_parts.push_back( std::move( part ) );
	}

	set( 0.F );
}

void Morph::set( float t )
{
	_t = t;
	for ( auto &part: _parts )
	{
		Path &path = *part.path;
		path.color = lerp( part.fill_a, part.fill_b, t );
		path.stroke_color = lerp( part.stroke_a, part.stroke_b, t );
		path.stroke_thickness = std::max( 0.F, lerp( part.thickness_a, part.thickness_b, t ) );

		path.commands.clear();
		for ( const auto &pair: part.contours )
		{
			path.commands.push_back( MoveTo{ lerp( pair.a.start, pair.b.start, t ) } );
			for ( size_t i = 0; i < pair.a.segs.size(); ++i )
			{
				const CubicSeg &a = pair.a.segs[i];
				const CubicSeg &b = pair.b.segs[i];
				path.commands.push_back( CubicTo{ lerp( a.to, b.to, t ), lerp( a.c1, b.c1, t ), lerp( a.c2, b.c2, t ) } );
			}
			path.commands.push_back( Close{} );
		}
	}
}
