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
		const OutlineShape *sa = pair.a >= 0 ? &shapes_a[static_cast<size_t>( pair.a )] : nullptr;
		const OutlineShape *sb = pair.b >= 0 ? &shapes_b[static_cast<size_t>( pair.b )] : nullptr;
		if ( ( sa && sa->image ) || ( sb && sb->image ) )
		{
			// A picture: the same one moves, a lone one fades (leaf matching never pairs it with anything else)
			part.image_a = sa ? *sa->image : *sb->image;
			part.image_b = sb ? *sb->image : *sa->image;
			part.fill_a = sa ? sa->fill : transparent( sb->fill );
			part.fill_b = sb ? sb->fill : transparent( sa->fill );
			auto node = std::make_unique<Image>();
			node->path = part.image_a.path;
			node->half_width = part.image_a.half_width;
			node->half_height = part.image_a.half_height;
			part.image = &_root->add( std::move( node ) );
			_parts.push_back( std::move( part ) );
			continue;
		}

		if ( pair.a >= 0 && pair.b >= 0 )
		{
			const auto &a = shapes_a[static_cast<size_t>( pair.a )];
			const auto &b = shapes_b[static_cast<size_t>( pair.b )];
			part.contours = match_contours( a, b, options.costs );
			part.original_a = a.contours;
			part.original_b = b.contours;
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
		// overshoots, it carries on past, from where it was at the end and at the same rate (not from its own
		// line, which with a stagger would jump: a part that finished early would leap ahead).
		const float rate = 1.F / ( 1.F - _stagger );
		float progress = std::clamp( ( _t - part.delay ) * rate, 0.F, 1.F );
		if ( _t > 1.F )
		{
			progress += ( _t - 1.F ) * rate;
		}
		else if ( _t < 0.F )
		{
			progress += _t * rate;
		}
		if ( _part_ease && progress >= 0.F && progress <= 1.F )
		{
			progress = _part_ease( progress );
		}

		if ( part.image )
		{
			Image &image = *part.image;
			image.position = lerp( part.image_a.position, part.image_b.position, progress );
			image.rotation = lerp( part.image_a.rotation, part.image_b.rotation, progress );
			image.scale_x = lerp( part.image_a.scale_x, part.image_b.scale_x, progress );
			image.scale_y = lerp( part.image_a.scale_y, part.image_b.scale_y, progress );
			image.color = lerp( part.fill_a, part.fill_b, progress );
			continue;
		}

		Path &path = *part.path;
		path.color = lerp( part.fill_a, part.fill_b, progress );
		path.stroke_color = lerp( part.stroke_a, part.stroke_b, progress );
		path.stroke_thickness = std::max( 0.F, lerp( part.thickness_a, part.thickness_b, progress ) );

		path.commands.clear();

		// Exactly at an end, the outline as it is drawn there
		const std::vector<Contour> *original = progress == 0.F ? &part.original_a : progress == 1.F ? &part.original_b : nullptr;
		if ( original && !original->empty() )
		{
			for ( const auto &contour: *original )
			{
				path.commands.push_back( MoveTo{ contour.start } );
				for ( const auto &seg: contour.segs )
				{
					path.commands.push_back( CubicTo{ seg.to, seg.c1, seg.c2 } );
				}
				if ( contour.closed )
				{
					path.commands.push_back( Close{} );
				}
			}
			continue;
		}

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
