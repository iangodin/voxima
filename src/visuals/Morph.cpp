#include "visuals/Morph.h"

#include <algorithm>
#include <cmath>
#include <numeric>

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

size_t open_contours( const OutlineShape &shape )
{
	return static_cast<size_t>( std::count_if( shape.contours.begin(), shape.contours.end(), []( const Contour &c ) { return !c.closed; } ) );
}

// Lengths on and off, repeated up to `size`
std::vector<float> dash_lengths( const std::vector<float> &pattern, size_t size )
{
	std::vector<float> out( size );
	for ( size_t i = 0; i < size; ++i )
	{
		out[i] = pattern[i % pattern.size()];
	}
	return out;
}

// How many lengths it takes to get back to a dash: an odd number of them only repeats after twice that
size_t even_size( const std::vector<float> &pattern )
{
	return pattern.size() % 2 == 0 ? pattern.size() : pattern.size() * 2;
}

// What the dashes are blended between. A whole line is the other side's dashes with no gaps, so gaps open up or close. Two
// patterns are repeated to the same number of lengths. Where a line becomes a filled shape it is drawn as a loop out and back
// (see closed_loop), whose dashes would fall on each other's gaps and look like a whole line anyway, so it is one.
template<typename Part>
void prepare_dashes( Part &part, const StrokeDash &a, const StrokeDash &b, bool loop )
{
	part.dash_a = a;
	part.dash_b = b;
	if ( loop || ( a.solid() && b.solid() ) )
	{
		return;
	}
	const auto no_gaps = []( std::vector<float> lengths )
	{
		for ( size_t i = 1; i < lengths.size(); i += 2 )
		{
			lengths[i] = 0.F;
		}
		return lengths;
	};
	if ( a.solid() || b.solid() )
	{
		const StrokeDash &dashes = a.solid() ? b : a;
		const auto lengths = dash_lengths( dashes.pattern, even_size( dashes.pattern ) );
		part.pattern_a = a.solid() ? no_gaps( lengths ) : lengths;
		part.pattern_b = b.solid() ? no_gaps( lengths ) : lengths;
		part.offset_a = part.offset_b = dashes.offset;
		part.cap_a = part.cap_b = dashes.cap;
		return;
	}
	const size_t size = std::lcm( even_size( a.pattern ), even_size( b.pattern ) );
	part.pattern_a = dash_lengths( a.pattern, size );
	part.pattern_b = dash_lengths( b.pattern, size );
	part.offset_a = a.offset;
	part.offset_b = b.offset;
	part.cap_a = a.cap;
	part.cap_b = b.cap;
}

// The same outline, invisible
Color transparent( Color c )
{
	c.a = 0.F;
	return c;
}

}

Point Similarity::apply( Point p, float t ) const
{
	if ( shift_only )
	{
		return p + shift * t;
	}
	const float s = std::exp( log_scale * t );
	const float c = std::cos( angle * t ) * s;
	const float n = std::sin( angle * t ) * s;
	const Point d = p - pivot;
	return pivot + Point{ c * d.x() - n * d.y(), n * d.x() + c * d.y() };
}

std::optional<Similarity> Similarity::between( const std::vector<Contour> &a, const std::vector<Contour> &b )
{
	std::vector<Point> pa;
	std::vector<Point> pb;
	if ( a.size() != b.size() )
	{
		return std::nullopt;
	}
	for ( size_t i = 0; i < a.size(); ++i )
	{
		if ( a[i].segs.size() != b[i].segs.size() || a[i].closed != b[i].closed )
		{
			return std::nullopt;
		}
		pa.push_back( a[i].start );
		pb.push_back( b[i].start );
		for ( size_t k = 0; k < a[i].segs.size(); ++k )
		{
			for ( auto [x, y]: { std::pair{ a[i].segs[k].c1, b[i].segs[k].c1 }, std::pair{ a[i].segs[k].c2, b[i].segs[k].c2 }, std::pair{ a[i].segs[k].to, b[i].segs[k].to } } )
			{
				pa.push_back( x );
				pb.push_back( y );
			}
		}
	}
	if ( pa.size() < 2 )
	{
		return std::nullopt;
	}

	// Least squares: centre both, then the turn and scale that best carry one onto the other
	Point ca;
	Point cb;
	for ( size_t i = 0; i < pa.size(); ++i )
	{
		ca += pa[i];
		cb += pb[i];
	}
	ca = ca / static_cast<float>( pa.size() );
	cb = cb / static_cast<float>( pb.size() );
	double dot = 0.0;
	double cross = 0.0;
	double norm = 0.0;
	for ( size_t i = 0; i < pa.size(); ++i )
	{
		const Point x = pa[i] - ca;
		const Point y = pb[i] - cb;
		dot += x.x() * y.x() + x.y() * y.y();
		cross += x.x() * y.y() - x.y() * y.x();
		norm += x.lengthSquared();
	}
	if ( norm <= 0.0 )
	{
		return std::nullopt;
	}
	const double angle = std::atan2( cross, dot );
	const double scale = std::hypot( dot, cross ) / norm;
	if ( scale <= 0.0 )
	{
		return std::nullopt;
	}

	// Only if it really is the same shape: every point lands (to a hair of the size) where it should
	const float c = static_cast<float>( std::cos( angle ) * scale );
	const float n = static_cast<float>( std::sin( angle ) * scale );
	const float size = static_cast<float>( std::sqrt( norm / static_cast<double>( pa.size() ) ) );
	for ( size_t i = 0; i < pa.size(); ++i )
	{
		const Point x = pa[i] - ca;
		const Point landed = cb + Point{ c * x.x() - n * x.y(), n * x.x() + c * x.y() };
		if ( ( landed - pb[i] ).lengthSquared() > 1e-8F * size * size )
		{
			return std::nullopt;
		}
	}

	Similarity out;
	if ( std::abs( angle ) < 1e-5 && std::abs( scale - 1.0 ) < 1e-5 )
	{
		out.shift = cb - ca;
		return out;
	}
	// The point that stays put: p = M p + T, where M turns and scales and T = cb - M ca
	const Point t = cb - Point{ c * ca.x() - n * ca.y(), n * ca.x() + c * ca.y() };
	const float det = ( 1.F - c ) * ( 1.F - c ) + n * n;
	out.shift_only = false;
	out.pivot = Point{ ( 1.F - c ) * t.x() - n * t.y(), n * t.x() + ( 1.F - c ) * t.y() } / det;
	out.angle = static_cast<float>( angle );
	out.log_scale = static_cast<float>( std::log( scale ) );
	return out;
}

Morph::Morph( const Visual &from, const Visual &to, const MorphOptions &options ) :
	_owned( std::make_unique<Group>() ),
	_root( _owned.get() ),
	_stagger( std::clamp( options.stagger, 0.F, 0.95F ) ),
	_part_ease( options.part_ease )
{
	const auto shapes_a = to_outline( from, nullptr, options.foreground );
	const auto shapes_b = to_outline( to, nullptr, options.foreground );

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
			part.original_a = a.contours;
			part.original_b = b.contours;
			if ( a.key == b.key )
			{
				part.carry = Similarity::between( a.contours, b.contours );
			}
			if ( !part.carry )
			{
				part.contours = match_contours( a, b, options.costs );
			}
			part.fill_a = a.fill;
			part.fill_b = b.fill;
			part.stroke_a = a.stroke;
			part.stroke_b = b.stroke;
			part.thickness_a = a.stroke_thickness;
			part.thickness_b = b.stroke_thickness;
			prepare_dashes( part, a.dash, b.dash, open_contours( a ) != open_contours( b ) );
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
			prepare_dashes( part, a.dash, a.dash, false );
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
			prepare_dashes( part, b.dash, b.dash, false );
		}
		auto path = std::make_unique<Path>();
		// A stroke (a Line, an open path) turning into or out of a filled shape is drawn as a loop out and back,
		// whose ends are corners that sharpen until they turn right round. Mitred, they would snap from points to
		// flat ends as they pass the mitre limit; bevelled, they are flat all along, which is also what the
		// stroke's own ends look like (a bevel on a turn right round is a butt cap).
		auto has_stroke = []( const OutlineShape *shape )
		{
			return shape && std::any_of( shape->contours.begin(), shape->contours.end(), []( const Contour &c ) { return !c.closed; } );
		};
		part.bevel = has_stroke( sa ) || has_stroke( sb );
		part.path = &_root->add( std::move( path ) );
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
		if ( progress == 0.F || progress == 1.F )
		{
			path.stroke_dash = progress == 0.F ? part.dash_a : part.dash_b;
		}
		else
		{
			path.stroke_dash.pattern.resize( part.pattern_a.size() );
			for ( size_t i = 0; i < part.pattern_a.size(); ++i )
			{
				path.stroke_dash.pattern[i] = std::max( 0.F, lerp( part.pattern_a[i], part.pattern_b[i], progress ) );
			}
			path.stroke_dash.offset = lerp( part.offset_a, part.offset_b, progress );
			path.stroke_dash.cap = progress < 0.5F ? part.cap_a : part.cap_b;
		}

		path.commands.clear();

		// Exactly at an end, the outline as it is drawn there
		const std::vector<Contour> *original = progress == 0.F ? &part.original_a : progress == 1.F ? &part.original_b : nullptr;
		// (the outlines at the ends are drawn as their own visuals are, with the default join)
		path.join = part.bevel && !( original && !original->empty() ) ? LineJoin::BEVEL : LineJoin::MITER;
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

		if ( part.carry )
		{
			for ( const auto &contour: part.original_a )
			{
				path.commands.push_back( MoveTo{ part.carry->apply( contour.start, progress ) } );
				for ( const auto &seg: contour.segs )
				{
					const Similarity &m = *part.carry;
					path.commands.push_back( CubicTo{ m.apply( seg.to, progress ), m.apply( seg.c1, progress ), m.apply( seg.c2, progress ) } );
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
