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
	_root( _owned.get() ),
	_stagger( std::clamp( options.stagger, 0.F, 0.95F ) ),
	_part_ease( options.part_ease )
{
	const auto shapes_a = to_outline( from );
	const auto shapes_b = to_outline( to );

	_pairs = match_leaves( shapes_a, shapes_b, options.leaves );
	for ( const LeafPair &pair: _pairs )
	{
		Part part;
		if ( pair.a >= 0 && pair.b >= 0 )
		{
			const auto &a = shapes_a[static_cast<size_t>( pair.a )];
			const auto &b = shapes_b[static_cast<size_t>( pair.b )];
			part.contours = match_contours( a, b, options.costs );
			part.fill_a = a.fill;
			part.fill_b = b.fill;
			part.stroke_a = a.stroke;
			part.stroke_b = b.stroke;
			part.thickness_a = a.stroke_thickness;
			part.thickness_b = b.stroke_thickness;
		}
		else if ( pair.a >= 0 )
		{
			// Nothing to become: fades out where it is
			const auto &a = shapes_a[static_cast<size_t>( pair.a )];
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
			const auto &b = shapes_b[static_cast<size_t>( pair.b )];
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

	// Parts start one after another, in drawing order
	for ( size_t k = 0; k < _parts.size(); ++k )
	{
		_parts[k].delay = _parts.size() > 1 ? _stagger * static_cast<float>( k ) / static_cast<float>( _parts.size() - 1 ) : 0.F;
	}

	set( 0.F );
}

void Morph::set( float t )
{
	_t = t;
	for ( auto &part: _parts )
	{
		// This part's own progress: it starts at its delay and takes what is left of the morph. Inside the
		// morph it stays between 0 and 1 (so it has finished before the end); outside, as when an ease
		// overshoots, it carries on past.
		float progress = ( _t - part.delay ) / ( 1.F - _stagger );
		if ( _t >= 0.F && _t <= 1.F )
		{
			progress = std::clamp( progress, 0.F, 1.F );
		}
		if ( _part_ease && progress >= 0.F && progress <= 1.F )
		{
			progress = _part_ease( progress );
		}

		Path &path = *part.path;
		path.color = lerp( part.fill_a, part.fill_b, progress );
		path.stroke_color = lerp( part.stroke_a, part.stroke_b, progress );
		path.stroke_thickness = std::max( 0.F, lerp( part.thickness_a, part.thickness_b, progress ) );

		path.commands.clear();
		for ( const auto &pair: part.contours )
		{
			path.commands.push_back( MoveTo{ lerp( pair.a.start, pair.b.start, progress ) } );
			for ( size_t i = 0; i < pair.a.segs.size(); ++i )
			{
				const CubicSeg &a = pair.a.segs[i];
				const CubicSeg &b = pair.b.segs[i];
				path.commands.push_back( CubicTo{ lerp( a.to, b.to, progress ), lerp( a.c1, b.c1, progress ), lerp( a.c2, b.c2, progress ) } );
			}
			if ( pair.a.closed )
			{
				path.commands.push_back( Close{} );
			}
		}
	}
}
