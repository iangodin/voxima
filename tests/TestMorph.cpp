#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/ArrowTip.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Group.h>
#include <visuals/Image.h>
#include <visuals/Line.h>
#include <visuals/Morph.h>
#include <visuals/MorphMatch.h>
#include <visuals/Outline.h>
#include <visuals/Path.h>
#include <visuals/Rectangle.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <variant>

using Catch::Approx;

namespace
{
Contour circle_contour( float radius = 1.F, Point center = { 0.F, 0.F } )
{
	Circle c;
	c.radius = radius;
	c.position = center;
	return to_outline( c )[0].contours[0];
}

Contour rect_contour( float hw, float hh )
{
	Rectangle r;
	r.half_width = hw;
	r.half_height = hh;
	return to_outline( r )[0].contours[0];
}

std::vector<Point> corners( const Contour &c )
{
	std::vector<Point> out{ c.start };
	for ( const auto &seg: c.segs )
	{
		out.push_back( seg.to );
	}
	out.pop_back(); // the last segment returns to the start
	return out;
}

bool same_point( Point a, Point b, float eps = 1e-3F )
{
	return std::abs( a.x() - b.x() ) < eps && std::abs( a.y() - b.y() ) < eps;
}

bool finite( const Morph &morph )
{
	for ( const auto &shape: to_outline( const_cast<Morph &>( morph ).visual() ) )
	{
		for ( const auto &c: shape.contours )
		{
			if ( !std::isfinite( c.start.x() ) || !std::isfinite( c.start.y() ) )
			{
				return false;
			}
			for ( const auto &s: c.segs )
			{
				for ( auto p: { s.c1, s.c2, s.to } )
				{
					if ( !std::isfinite( p.x() ) || !std::isfinite( p.y() ) )
					{
						return false;
					}
				}
			}
		}
	}
	return true;
}

// Total area of everything the morph draws (holes count negative, as they wind the other way)
double area_of( Visual &visual )
{
	double total = 0.0;
	for ( const auto &shape: to_outline( visual ) )
	{
		for ( const auto &c: shape.contours )
		{
			total += signed_area( c );
		}
	}
	return total;
}

AABB bounds_of( Visual &visual )
{
	AABB box;
	for ( const auto &shape: to_outline( visual ) )
	{
		for ( const auto &c: shape.contours )
		{
			box.add( on_curve_bounds( c ) );
		}
	}
	return box;
}
}

namespace
{
// Angle (radians, 0..pi) between two points as seen from the origin
float angle_between( Point a, Point b )
{
	const float d = std::atan2( a.y(), a.x() ) - std::atan2( b.y(), b.x() );
	return std::abs( std::remainder( d, 2.F * std::numbers::pi_v<float> ) );
}

// The largest angle, around the origin, that a point moves when its contour is morphed into the other
float largest_twist( const ContourPair &pair )
{
	const auto a = corners( pair.a );
	const auto b = corners( pair.b );
	float worst = 0.F;
	for ( size_t i = 0; i < a.size(); ++i )
	{
		worst = std::max( worst, angle_between( a[i], b[i] ) );
	}
	return worst;
}
}

TEST_CASE( "align_contours does not twist a circle into a square", "[morph]" )
{
	const Contour circle = circle_contour( 1.F );

	SECTION( "square" )
	{
		const ContourPair pair = align_contours( circle, rect_contour( 1.F, 1.F ) );
		REQUIRE( pair.a.segs.size() == pair.b.segs.size() );
		// the circle's extrema go to the square's edge midpoints, and its 45 degree points to the corners
		CHECK( largest_twist( pair ) < 0.05F );
	}

	SECTION( "wide rectangle" )
	{
		const ContourPair pair = align_contours( circle, rect_contour( 2.F, 1.F ) );
		CHECK( largest_twist( pair ) < 0.1F );
	}

	SECTION( "and back again" )
	{
		const ContourPair pair = align_contours( rect_contour( 1.F, 1.F ), circle );
		CHECK( largest_twist( pair ) < 0.05F );
	}

	SECTION( "wherever the outlines start" )
	{
		// The same square, starting from a different corner
		const Contour square = rect_contour( 1.F, 1.F );
		Contour shifted;
		shifted.start = square.segs[1].to;
		shifted.segs = { square.segs[2], square.segs[3], square.segs[0], square.segs[1] };
		CHECK( largest_twist( align_contours( circle, shifted ) ) < 0.05F );
	}
}

TEST_CASE( "align_contours pairs corners with corners when both shapes have them", "[morph]" )
{
	// A square into a diamond (the same square turned 45 degrees) has four corners each: they pair up
	// in order, one quarter of the way round from where the diamond's are, rather than being invented
	const Contour square = rect_contour( 1.F, 1.F );
	Rectangle turned;
	turned.half_width = turned.half_height = 1.F;
	turned.rotation = 45.F;
	const Contour diamond = to_outline( turned )[0].contours[0];

	const ContourPair pair = align_contours( square, diamond );
	CHECK( pair.a.segs.size() == pair.b.segs.size() );
	CHECK( signed_area( pair.a ) == Approx( signed_area( square ) ).epsilon( 1e-4 ) );
	CHECK( signed_area( pair.b ) == Approx( signed_area( diamond ) ).epsilon( 1e-4 ) );
}

TEST_CASE( "align_contours keeps both shapes exactly as they were", "[morph]" )
{
	const Contour circle = circle_contour( 1.5F, { 2.F, 1.F } );
	const Contour rect = rect_contour( 3.F, 0.5F );

	const ContourPair pair = align_contours( circle, rect );

	CHECK( pair.a.segs.size() == pair.b.segs.size() );
	CHECK( signed_area( pair.a ) == Approx( signed_area( circle ) ).epsilon( 1e-4 ) );
	CHECK( signed_area( pair.b ) == Approx( signed_area( rect ) ).epsilon( 1e-4 ) );
	CHECK( pair.a.segs.back().to == pair.a.start );
	CHECK( pair.b.segs.back().to == pair.b.start );
}

TEST_CASE( "align_contours splits the contour with fewer features", "[morph]" )
{
	// An octagon-ish contour has 8 corners; the circle only 4 extrema, so it is split
	Contour octagon;
	const int n = 8;
	octagon.start = { 1.F, 0.F };
	for ( int i = 1; i <= n; ++i )
	{
		const float a = 2.F * std::numbers::pi_v<float> * static_cast<float>( i ) / n;
		const Point from = i == 1 ? octagon.start : octagon.segs.back().to;
		const Point to = i == n ? octagon.start : Point{ std::cos( a ), std::sin( a ) };
		octagon.segs.push_back( { from + ( to - from ) * ( 1.F / 3.F ), from + ( to - from ) * ( 2.F / 3.F ), to } );
	}

	const ContourPair pair = align_contours( circle_contour( 1.F ), octagon );

	CHECK( pair.a.segs.size() == pair.b.segs.size() );
	CHECK( pair.a.segs.size() >= 8 );
	CHECK( pair.b.segs.size() == 8 ); // the octagon's corners are all kept
	CHECK( signed_area( pair.b ) == Approx( signed_area( octagon ) ).epsilon( 1e-4 ) );
}

TEST_CASE( "point_contour is a contour that is one point", "[morph]" )
{
	const Contour p = point_contour( { 1.F, 2.F }, 5 );
	REQUIRE( p.segs.size() == 5 );
	CHECK( signed_area( p ) == Approx( 0.0 ) );
	CHECK( same_point( p.segs[3].c1, { 1.F, 2.F } ) );
}

TEST_CASE( "match_contours pairs holes with holes and outers with outers", "[morph]" )
{
	auto theme = AcademicClassic::create_theme();
	const FontStyle style{ Intent::PROSE };

	auto o = to_outline( *make_text( theme, "o", style ) );
	auto e = to_outline( *make_text( theme, "e", style ) );
	REQUIRE( o.size() == 1 );
	REQUIRE( e.size() == 1 );

	const auto pairs = match_contours( o[0], e[0] );
	REQUIRE( pairs.size() == o[0].contours.size() ); // both have an outer and a hole
	REQUIRE( pairs.size() == 2 );

	for ( const auto &pair: pairs )
	{
		CHECK( pair.a.segs.size() == pair.b.segs.size() );
		// an outer never pairs with a hole
		CHECK( signed_area( pair.a ) * signed_area( pair.b ) > 0.0 );
	}
}

TEST_CASE( "match_contours makes windings agree", "[morph]" )
{
	// a circle wound the other way still ends up with the same winding as the square
	OutlineShape circle;
	circle.contours.push_back( reversed( circle_contour( 1.F ) ) );
	OutlineShape square;
	square.contours.push_back( rect_contour( 1.F, 1.F ) );

	const auto pairs = match_contours( circle, square );
	REQUIRE( pairs.size() == 1 );
	CHECK( signed_area( pairs[0].a ) * signed_area( pairs[0].b ) > 0.0 );
}

TEST_CASE( "match_contours shrinks a contour with no partner into a point", "[morph]" )
{
	auto theme = AcademicClassic::create_theme();
	const FontStyle style{ Intent::PROSE };
	auto o = to_outline( *make_text( theme, "o", style ) );
	auto x = to_outline( *make_text( theme, "x", style ) );

	const auto pairs = match_contours( o[0], x[0] ); // 2 contours vs 1
	REQUIRE( pairs.size() == 2 );

	size_t collapsed = 0;
	for ( const auto &pair: pairs )
	{
		CHECK( pair.a.segs.size() == pair.b.segs.size() );
		if ( std::abs( signed_area( pair.b ) ) < 1e-9 )
		{
			++collapsed;
		}
	}
	CHECK( collapsed == 1 ); // the hole of the "o"
}

TEST_CASE( "Morph starts as the source and ends as the target", "[morph]" )
{
	Circle from;
	from.radius = 1.F;
	from.color = { 1.F, 0.F, 0.F, 1.F };
	Rectangle to;
	to.half_width = 2.F;
	to.half_height = 1.F;
	to.position = { 4.F, 1.F };
	to.color = { 0.F, 0.F, 1.F, 1.F };

	Morph morph( from, to );

	SECTION( "t = 0" )
	{
		morph.set( 0.F );
		CHECK( area_of( morph.visual() ) == Approx( std::numbers::pi ).epsilon( 1e-3 ) );
		const AABB box = bounds_of( morph.visual() );
		CHECK( box.min.x() == Approx( -1.F ).margin( 1e-3 ) );
		CHECK( box.max.x() == Approx( 1.F ).margin( 1e-3 ) );
		CHECK( to_outline( morph.visual() )[0].fill == Color{ 1.F, 0.F, 0.F, 1.F } );
	}

	SECTION( "t = 1" )
	{
		morph.set( 1.F );
		CHECK( area_of( morph.visual() ) == Approx( 8.0 ).epsilon( 1e-3 ) );
		const AABB box = bounds_of( morph.visual() );
		CHECK( box.min.x() == Approx( 2.F ).margin( 1e-3 ) );
		CHECK( box.max.x() == Approx( 6.F ).margin( 1e-3 ) );
		CHECK( box.min.y() == Approx( 0.F ).margin( 1e-3 ) );
		CHECK( box.max.y() == Approx( 2.F ).margin( 1e-3 ) );
		CHECK( to_outline( morph.visual() )[0].fill == Color{ 0.F, 0.F, 1.F, 1.F } );
	}

	SECTION( "halfway blends position and color" )
	{
		morph.set( 0.5F );
		const AABB box = bounds_of( morph.visual() );
		// the shapes' centres are (0,0) and (4,1)
		CHECK( ( box.min.x() + box.max.x() ) / 2.F == Approx( 2.F ).margin( 0.05 ) );
		CHECK( ( box.min.y() + box.max.y() ) / 2.F == Approx( 0.5F ).margin( 0.05 ) );
		const Color c = to_outline( morph.visual() )[0].fill;
		CHECK( c.r == Approx( 0.5F ) );
		CHECK( c.b == Approx( 0.5F ) );
	}

	SECTION( "extrapolating stays finite and keeps colors valid" )
	{
		for ( float t: { -0.5F, -0.2F, 1.2F, 1.5F } )
		{
			morph.set( t );
			CHECK( finite( morph ) );
			const Color c = to_outline( morph.visual() )[0].fill;
			CHECK( c.r >= 0.F );
			CHECK( c.r <= 1.F );
			CHECK( c.b >= 0.F );
			CHECK( c.b <= 1.F );
		}
		morph.set( 1.2F );
		CHECK( morph.t() == 1.2F );
	}
}

TEST_CASE( "Morph between glyphs", "[morph]" )
{
	auto theme = AcademicClassic::create_theme();
	const FontStyle style{ Intent::PROSE };

	auto glyph = [&]( const char *s ) { return make_text( theme, s, style ); };

	SECTION( "every pair of letters morphs cleanly, start to end" )
	{
		for ( const char *a: { "o", "x", "e", "a", "8", "l" } )
		{
			for ( const char *b: { "o", "x", "e", "a", "8", "l" } )
			{
				auto from = glyph( a );
				auto to = glyph( b );
				Morph morph( *from, *to );
				INFO( a << " -> " << b );

				// (a morph may wind the target the other way, which flips the sign, not the shape)
				morph.set( 0.F );
				CHECK( std::abs( area_of( morph.visual() ) ) == Approx( std::abs( area_of( *from ) ) ).epsilon( 1e-3 ) );
				morph.set( 1.F );
				CHECK( std::abs( area_of( morph.visual() ) ) == Approx( std::abs( area_of( *to ) ) ).epsilon( 1e-3 ) );

				for ( float t: { 0.25F, 0.5F, 0.75F, -0.2F, 1.2F } )
				{
					morph.set( t );
					CHECK( finite( morph ) );
				}
			}
		}
	}

	SECTION( "an o turning into an x collapses its hole, and it grows back" )
	{
		auto o = glyph( "o" );
		auto x = glyph( "x" );
		Morph forward( *o, *x );
		Morph back( *x, *o );

		forward.set( 1.F );
		back.set( 0.F );
		// same drawing: one contour is a point at the end / start
		CHECK( std::abs( area_of( forward.visual() ) ) == Approx( std::abs( area_of( *x ) ) ).epsilon( 1e-3 ) );
		CHECK( std::abs( area_of( back.visual() ) ) == Approx( std::abs( area_of( *x ) ) ).epsilon( 1e-3 ) );
	}

	SECTION( "a glyph morphing into itself stays put" )
	{
		auto a = glyph( "a" );
		auto b = glyph( "a" );
		Morph morph( *a, *b );

		morph.set( 0.F );
		const AABB start = bounds_of( morph.visual() );
		const double area = area_of( morph.visual() );
		morph.set( 0.5F );
		const AABB mid = bounds_of( morph.visual() );
		CHECK( mid.min.x() == Approx( start.min.x() ).margin( 1e-4 ) );
		CHECK( mid.max.y() == Approx( start.max.y() ).margin( 1e-4 ) );
		CHECK( area_of( morph.visual() ) == Approx( area ).epsilon( 1e-4 ) );
	}
}

TEST_CASE( "Morph handles visuals with different numbers of leaves", "[morph]" )
{
	auto theme = AcademicClassic::create_theme();
	auto one = make_text( theme, "a", FontStyle{ Intent::PROSE } );
	auto two = make_text( theme, "ab", FontStyle{ Intent::PROSE } );

	SECTION( "an extra leaf fades in" )
	{
		Morph morph( *one, *two );
		morph.set( 0.F );
		auto shapes = to_outline( morph.visual() );
		REQUIRE( shapes.size() == 2 );
		CHECK( shapes[1].fill.a == Approx( 0.F ) );
		morph.set( 1.F );
		CHECK( to_outline( morph.visual() )[1].fill.a == Approx( 1.F ) );
	}

	SECTION( "a missing leaf fades out where it was" )
	{
		Morph morph( *two, *one );
		morph.set( 1.F );
		auto shapes = to_outline( morph.visual() );
		REQUIRE( shapes.size() == 2 );
		CHECK( shapes[1].fill.a == Approx( 0.F ) );
		CHECK( shapes[0].fill.a == Approx( 1.F ) );
	}
}

TEST_CASE( "Morph::release hands over the drawing but keeps it updating", "[morph]" )
{
	Circle from;
	Rectangle to;
	to.half_width = 3.F;
	to.half_height = 3.F;
	Morph morph( from, to );

	std::unique_ptr<Group> group = morph.release();
	REQUIRE( group );
	CHECK( group.get() == &morph.visual() );

	morph.set( 1.F );
	CHECK( area_of( *group ) == Approx( 36.0 ).epsilon( 1e-3 ) );
}

TEST_CASE( "Group::replace and remove", "[morph]" )
{
	Group group;
	auto &a = group.add( std::make_unique<Circle>() );
	auto &b = group.add( std::make_unique<Rectangle>() );
	auto &c = group.add( std::make_unique<Circle>() );

	SECTION( "replace keeps the drawing order and returns the old one" )
	{
		auto replacement = std::make_unique<Rectangle>();
		Visual *replacement_ptr = replacement.get();
		auto old = group.replace( &b, std::move( replacement ) );
		CHECK( old.get() == &b );
		REQUIRE( group.size() == 3 );
		auto it = group.begin();
		CHECK( it->get() == &a );
		CHECK( ( it + 1 )->get() == replacement_ptr );
		CHECK( ( it + 2 )->get() == &c );
	}

	SECTION( "replace of something that is not a child changes nothing" )
	{
		Circle stranger;
		auto old = group.replace( &stranger, std::make_unique<Circle>() );
		CHECK_FALSE( old );
		CHECK( group.size() == 3 );
	}

	SECTION( "remove takes the child out" )
	{
		auto taken = group.remove( &b );
		CHECK( taken.get() == &b );
		CHECK( group.size() == 2 );
		CHECK_FALSE( group.remove( &b ) );
	}
}

namespace
{
Contour polyline( std::initializer_list<Point> points )
{
	Contour c;
	c.closed = false;
	auto it = points.begin();
	c.start = *it;
	Point prev = *it;
	for ( ++it; it != points.end(); ++it )
	{
		c.segs.push_back( { prev + ( *it - prev ) * ( 1.F / 3.F ), prev + ( *it - prev ) * ( 2.F / 3.F ), *it } );
		prev = *it;
	}
	return c;
}

Point end_of( const Contour &c )
{
	return c.segs.back().to;
}
}

TEST_CASE( "align_open cuts two strokes to the same number of pieces, ends fixed", "[morph]" )
{
	const Contour straight = polyline( { { 0.F, 0.F }, { 3.F, 0.F } } );
	const Contour bent = polyline( { { 0.F, 0.F }, { 1.F, 0.F }, { 1.F, 2.F }, { 3.F, 2.F } } );

	const ContourPair pair = align_open( straight, bent );

	CHECK_FALSE( pair.a.closed );
	CHECK_FALSE( pair.b.closed );
	CHECK( pair.a.segs.size() == pair.b.segs.size() );
	CHECK( pair.a.segs.size() >= 3 );
	CHECK( same_point( pair.a.start, { 0.F, 0.F } ) );
	CHECK( same_point( pair.b.start, { 0.F, 0.F } ) );
	CHECK( same_point( end_of( pair.a ), { 3.F, 0.F } ) );
	CHECK( same_point( end_of( pair.b ), { 3.F, 2.F } ) );

	SECTION( "neither stroke changes shape" )
	{
		// every point of the straight one is still on the line
		Point at = pair.a.start;
		for ( const auto &seg: pair.a.segs )
		{
			CHECK( std::abs( seg.c1.y() ) < 1e-4F );
			CHECK( std::abs( seg.c2.y() ) < 1e-4F );
			CHECK( seg.to.x() >= at.x() - 1e-4F );
			at = seg.to;
		}
		// and the bent one still passes through its corners
		bool corner1 = false;
		bool corner2 = false;
		for ( const auto &seg: pair.b.segs )
		{
			corner1 = corner1 || same_point( seg.to, { 1.F, 0.F } );
			corner2 = corner2 || same_point( seg.to, { 1.F, 2.F } );
		}
		CHECK( corner1 );
		CHECK( corner2 );
	}
}

TEST_CASE( "align_open of two single segments needs no cutting", "[morph]" )
{
	const ContourPair pair = align_open( polyline( { { 0.F, 0.F }, { 1.F, 0.F } } ), polyline( { { 0.F, 1.F }, { 5.F, 4.F } } ) );
	CHECK( pair.a.segs.size() == 1 );
	CHECK( pair.b.segs.size() == 1 );
}

TEST_CASE( "match_contours pairs strokes with strokes, and a lone stroke with a loop", "[morph]" )
{
	OutlineShape stroke_a;
	stroke_a.contours.push_back( polyline( { { 0.F, 0.F }, { 2.F, 0.F } } ) );
	OutlineShape stroke_b;
	stroke_b.contours.push_back( polyline( { { 0.F, 1.F }, { 2.F, 3.F } } ) );
	OutlineShape disc;
	disc.contours.push_back( circle_contour( 1.F ) );

	SECTION( "stroke to stroke stays open" )
	{
		const auto pairs = match_contours( stroke_a, stroke_b );
		REQUIRE( pairs.size() == 1 );
		CHECK_FALSE( pairs[0].a.closed );
		CHECK_FALSE( pairs[0].b.closed );
	}

	SECTION( "stroke to a filled shape goes out and back, then morphs like any loop" )
	{
		const auto pairs = match_contours( stroke_a, disc );
		REQUIRE( pairs.size() == 1 );
		CHECK( pairs[0].a.closed );
		CHECK( pairs[0].b.closed );
		CHECK( pairs[0].a.segs.size() == pairs[0].b.segs.size() );
		CHECK( signed_area( pairs[0].a ) == Approx( 0.0 ).margin( 1e-9 ) );
		CHECK( std::abs( signed_area( pairs[0].b ) ) == Approx( std::numbers::pi ).epsilon( 1e-3 ) );
	}

	SECTION( "and the other way round" )
	{
		const auto pairs = match_contours( disc, stroke_a );
		REQUIRE( pairs.size() == 1 );
		CHECK( pairs[0].a.closed );
		CHECK( pairs[0].b.closed );
	}
}

TEST_CASE( "A line morphs into a disc and back", "[morph]" )
{
	Line line;
	line.start = { -2.F, 0.F };
	line.end = { 2.F, 0.F };
	line.stroke_thickness = 0.2F;
	line.stroke_color = { 1.F, 1.F, 0.F, 1.F };
	Circle disc;
	disc.radius = 1.F;
	disc.color = { 1.F, 0.F, 0.F, 1.F };

	Morph morph( line, disc );

	morph.set( 0.F );
	auto start = to_outline( morph.visual() );
	REQUIRE( start.size() == 1 );
	CHECK( start[0].fill.a == Approx( 0.F ) );
	CHECK( start[0].stroke_thickness == Approx( 0.2F ) );
	CHECK( std::abs( area_of( morph.visual() ) ) == Approx( 0.0 ).margin( 1e-6 ) );
	CHECK( bounds_of( morph.visual() ).min.x() == Approx( -2.F ).margin( 1e-3 ) );
	CHECK( bounds_of( morph.visual() ).max.x() == Approx( 2.F ).margin( 1e-3 ) );

	morph.set( 1.F );
	auto end = to_outline( morph.visual() );
	CHECK( end[0].fill.a == Approx( 1.F ) );
	CHECK( end[0].stroke_thickness == Approx( 0.F ).margin( 1e-6 ) );
	CHECK( std::abs( area_of( morph.visual() ) ) == Approx( std::numbers::pi ).epsilon( 1e-3 ) );

	for ( float t: { 0.25F, 0.5F, 0.75F, -0.2F, 1.2F } )
	{
		morph.set( t );
		CHECK( finite( morph ) );
	}

	Morph back( disc, line );
	back.set( 1.F );
	CHECK( std::abs( area_of( back.visual() ) ) == Approx( 0.0 ).margin( 1e-6 ) );
}

TEST_CASE( "A line morphed into a line stays an open stroke", "[morph]" )
{
	Line a;
	a.start = { 0.F, 0.F };
	a.end = { 2.F, 0.F };
	a.stroke_thickness = 0.1F;
	Line b;
	b.start = { 0.F, 1.F };
	b.end = { 3.F, 2.F };
	b.stroke_thickness = 0.3F;

	Morph morph( a, b );
	morph.set( 0.5F );

	REQUIRE( morph.visual().size() == 1 );
	const auto &path = static_cast<const Path &>( **morph.visual().begin() );
	bool closes = false;
	for ( const auto &cmd: path.commands )
	{
		closes = closes || std::holds_alternative<Close>( cmd );
	}
	CHECK_FALSE( closes );
	CHECK( path.stroke_thickness == Approx( 0.2F ) );
	CHECK( path.color.a == Approx( 0.F ) );
}

TEST_CASE( "A group with lines and an arrow head keeps them through a morph", "[morph]" )
{
	auto arrow = []( float dx, float dy )
	{
		auto g = std::make_unique<Group>();
		auto line = std::make_unique<Line>();
		line->end = { dx, dy };
		line->stroke_thickness = 0.1F;
		auto tip = std::make_unique<ArrowTip>();
		tip->end = { dx, dy };
		tip->size = 0.2F;
		g->add( std::move( line ) );
		g->add( std::move( tip ) );
		return g;
	};

	auto right = arrow( 3.F, 0.F );
	auto up = arrow( 0.F, 3.F );

	Morph morph( *right, *up );
	// the stroke turns into the stroke, the tip into the tip
	REQUIRE( morph.visual().size() == 2 );
	for ( auto p: morph.pairs() )
	{
		CHECK( p.a >= 0 );
		CHECK( p.b >= 0 );
	}
	morph.set( 0.F );
	CHECK( bounds_of( morph.visual() ).max.x() == Approx( 3.F ).margin( 1e-3 ) );
	morph.set( 1.F );
	CHECK( bounds_of( morph.visual() ).max.y() == Approx( 3.F ).margin( 1e-3 ) );
}

TEST_CASE( "align_contours copes with dense outlines and stays exact", "[morph]" )
{
	// A 300-corner polygon around a circle against a 260-corner one around a wobbly ring
	auto ring = []( int n, float radius, float wobble )
	{
		Contour c;
		auto at = [&]( int i )
		{
			const float a = 2.F * std::numbers::pi_v<float> * static_cast<float>( i ) / static_cast<float>( n );
			const float r = radius * ( 1.F + wobble * std::sin( 7.F * a ) );
			return Point{ r * std::cos( a ), r * std::sin( a ) };
		};
		c.start = at( 0 );
		Point prev = c.start;
		for ( int i = 1; i <= n; ++i )
		{
			const Point to = i == n ? c.start : at( i );
			c.segs.push_back( { prev + ( to - prev ) * ( 1.F / 3.F ), prev + ( to - prev ) * ( 2.F / 3.F ), to } );
			prev = to;
		}
		return c;
	};
	const Contour a = ring( 300, 1.F, 0.F );
	const Contour b = ring( 260, 1.5F, 0.2F );

	const ContourPair pair = align_contours( a, b );
	CHECK( pair.a.segs.size() == pair.b.segs.size() );
	CHECK( pair.a.segs.size() >= 300 );
	CHECK( signed_area( pair.a ) == Approx( signed_area( a ) ).epsilon( 1e-4 ) );
	CHECK( signed_area( pair.b ) == Approx( signed_area( b ) ).epsilon( 1e-4 ) );

	SECTION( "trying only a few anchors gives an equally good pairing" )
	{
		AlignCosts few;
		few.max_anchors = 2;
		const ContourPair cheap = align_contours( a, b, few );
		CHECK( cheap.a.segs.size() == cheap.b.segs.size() );
		CHECK( signed_area( cheap.a ) == Approx( signed_area( a ) ).epsilon( 1e-4 ) );
		CHECK( signed_area( cheap.b ) == Approx( signed_area( b ) ).epsilon( 1e-4 ) );
	}
}

namespace
{
// Three discs in a row, and three rectangles in the same places
std::unique_ptr<Group> discs()
{
	auto g = std::make_unique<Group>();
	for ( int i = 0; i < 3; ++i )
	{
		auto c = std::make_unique<Circle>();
		c->radius = 1.F;
		c->position = { 5.F * static_cast<float>( i ), 0.F };
		g->add( std::move( c ) );
	}
	return g;
}

std::unique_ptr<Group> boxes()
{
	auto g = std::make_unique<Group>();
	for ( int i = 0; i < 3; ++i )
	{
		auto r = std::make_unique<Rectangle>();
		r->half_width = 1.F;
		r->half_height = 0.5F;
		r->position = { 5.F * static_cast<float>( i ), 0.F };
		g->add( std::move( r ) );
	}
	return g;
}

double part_area( Morph &morph, size_t k )
{
	return std::abs( signed_area( to_outline( morph.visual() )[k].contours[0] ) );
}
}

TEST_CASE( "Morph stagger starts the parts one after another", "[morph]" )
{
	auto from = discs();
	auto to = boxes();

	MorphOptions options;
	options.stagger = 0.5F;
	Morph morph( *from, *to, options );
	REQUIRE( morph.visual().size() == 3 );

	const double disc = std::numbers::pi;
	const double box = 4.0 * 1.0 * 0.5;

	SECTION( "the ends are the source and the target, whatever the stagger" )
	{
		morph.set( 0.F );
		for ( size_t k = 0; k < 3; ++k ) CHECK( part_area( morph, k ) == Approx( disc ).epsilon( 1e-3 ) );
		morph.set( 1.F );
		for ( size_t k = 0; k < 3; ++k ) CHECK( part_area( morph, k ) == Approx( box ).epsilon( 1e-3 ) );
	}

	SECTION( "halfway, the first part is done, the last has not begun" )
	{
		morph.set( 0.5F );
		CHECK( part_area( morph, 0 ) == Approx( box ).epsilon( 1e-3 ) );
		CHECK( part_area( morph, 2 ) == Approx( disc ).epsilon( 1e-3 ) );
		// the middle one is half way there
		const double mid = part_area( morph, 1 );
		CHECK( mid < disc );
		CHECK( mid > box );
	}

	SECTION( "parts that start later are behind" )
	{
		morph.set( 0.3F );
		// 0.3 is 60% through the first part's span, 10% through the middle one's, not yet the last's
		const double a = part_area( morph, 0 );
		const double b = part_area( morph, 1 );
		const double c = part_area( morph, 2 );
		CHECK( a < b );
		CHECK( b < c );
		CHECK( c == Approx( disc ).epsilon( 1e-3 ) );
	}

	SECTION( "overshooting easing carries on past the ends" )
	{
		for ( float t: { -0.3F, 1.3F } )
		{
			morph.set( t );
			CHECK( finite( morph ) );
		}
	}
}

TEST_CASE( "Morph without stagger moves every part together", "[morph]" )
{
	auto from = discs();
	auto to = boxes();
	Morph morph( *from, *to );
	morph.set( 0.5F );
	CHECK( part_area( morph, 0 ) == Approx( part_area( morph, 1 ) ).epsilon( 1e-4 ) );
	CHECK( part_area( morph, 1 ) == Approx( part_area( morph, 2 ) ).epsilon( 1e-4 ) );
}

TEST_CASE( "Morph part_ease reshapes each part's own progress", "[morph]" )
{
	auto from = discs();
	auto to = boxes();

	MorphOptions options;
	// nothing happens until a part is completely done, then it jumps
	options.part_ease = []( float u ) { return u < 1.F ? 0.F : 1.F; };
	Morph morph( *from, *to, options );

	morph.set( 0.5F );
	for ( size_t k = 0; k < 3; ++k )
	{
		CHECK( part_area( morph, k ) == Approx( std::numbers::pi ).epsilon( 1e-3 ) );
	}
	morph.set( 1.F );
	for ( size_t k = 0; k < 3; ++k )
	{
		CHECK( part_area( morph, k ) == Approx( 2.0 ).epsilon( 1e-3 ) );
	}
}

TEST_CASE( "Morph stagger outside its range is safe", "[morph]" )
{
	auto from = discs();
	auto to = boxes();
	for ( float stagger: { -1.F, 5.F } )
	{
		MorphOptions options;
		options.stagger = stagger;
		Morph morph( *from, *to, options );
		for ( float t: { 0.F, 0.5F, 1.F } )
		{
			morph.set( t );
			CHECK( finite( morph ) );
		}
	}
}

namespace
{
std::unique_ptr<Image> picture( const char *path, Point at, float alpha = 1.F )
{
	auto image = std::make_unique<Image>();
	image->path = path;
	image->half_width = 1.6F;
	image->half_height = 0.9F;
	image->position = at;
	image->color = { 1.F, 1.F, 1.F, alpha };
	return image;
}

const Image &image_part( Morph &morph, size_t k )
{
	return dynamic_cast<const Image &>( **( morph.visual().begin() + static_cast<std::ptrdiff_t>( k ) ) );
}
}

TEST_CASE( "Morph moves the same picture and cross-fades different ones", "[morph]" )
{
	SECTION( "the same picture moves, staying as it is" )
	{
		auto from = picture( "a.png", { 0.F, 0.F } );
		auto to = picture( "a.png", { 4.F, 2.F } );
		Morph morph( *from, *to );

		REQUIRE( morph.visual().size() == 1 );
		morph.set( 0.5F );
		CHECK( image_part( morph, 0 ).position.x() == Approx( 2.F ) );
		CHECK( image_part( morph, 0 ).position.y() == Approx( 1.F ) );
		CHECK( image_part( morph, 0 ).color.a == Approx( 1.F ) );
		CHECK( image_part( morph, 0 ).path == "a.png" );
	}

	SECTION( "a different picture fades out while the new one fades in" )
	{
		auto from = picture( "a.png", { 0.F, 0.F } );
		auto to = picture( "b.png", { 0.F, 0.F } );
		Morph morph( *from, *to );

		REQUIRE( morph.visual().size() == 2 );
		morph.set( 0.F );
		CHECK( image_part( morph, 0 ).color.a == Approx( 1.F ) );
		CHECK( image_part( morph, 1 ).color.a == Approx( 0.F ) );
		morph.set( 0.25F );
		CHECK( image_part( morph, 0 ).color.a == Approx( 0.75F ) );
		CHECK( image_part( morph, 1 ).color.a == Approx( 0.25F ) );
		morph.set( 1.F );
		CHECK( image_part( morph, 0 ).color.a == Approx( 0.F ) );
		CHECK( image_part( morph, 1 ).color.a == Approx( 1.F ) );
		CHECK( image_part( morph, 0 ).path == "a.png" );
		CHECK( image_part( morph, 1 ).path == "b.png" );
	}

	SECTION( "a picture and a shape cross-fade too" )
	{
		auto from = picture( "a.png", { 0.F, 0.F } );
		Circle disc;
		Morph morph( *from, disc );

		REQUIRE( morph.visual().size() == 2 );
		morph.set( 0.F );
		// the picture is still there, the disc not yet visible
		const auto shapes = to_outline( morph.visual() );
		REQUIRE( shapes.size() == 2 );
		CHECK( shapes[0].image.has_value() );
		CHECK( shapes[0].fill.a == Approx( 1.F ) );
		CHECK( shapes[1].fill.a == Approx( 0.F ) );
		morph.set( 1.F );
		const auto done = to_outline( morph.visual() );
		CHECK( done[0].fill.a == Approx( 0.F ) );
		CHECK( done[1].fill.a == Approx( 1.F ) );
	}

	SECTION( "extrapolating stays sensible" )
	{
		auto from = picture( "a.png", { 0.F, 0.F } );
		auto to = picture( "b.png", { 0.F, 0.F } );
		Morph morph( *from, *to );
		for ( float t: { -0.5F, 1.5F } )
		{
			morph.set( t );
			CHECK( image_part( morph, 0 ).color.a >= 0.F );
			CHECK( image_part( morph, 0 ).color.a <= 1.F );
		}
	}
}

namespace
{
// Points along a contour, `per_seg` to a segment
std::vector<Point> trace( const Contour &c, int per_seg = 24 )
{
	std::vector<Point> out;
	Point p0 = c.start;
	for ( const auto &s: c.segs )
	{
		for ( int k = 0; k < per_seg; ++k )
		{
			const float t = static_cast<float>( k ) / static_cast<float>( per_seg );
			const float u = 1.F - t;
			out.push_back( p0 * ( u * u * u ) + s.c1 * ( 3.F * u * u * t ) + s.c2 * ( 3.F * u * t * t ) + s.to * ( t * t * t ) );
		}
		p0 = s.to;
	}
	return out;
}

// How far the furthest point of one contour is from the other (both ways): 0 if they are the same curve
float distance_between( const Contour &x, const Contour &y )
{
	const auto px = trace( x, 48 );
	const auto py = trace( y, 48 );
	// Distance from each point of one to the other traced as a closed polyline (its chords are far finer than
	// the error being looked for)
	auto one_way = [&]( const std::vector<Point> &from, const std::vector<Point> &to )
	{
		float worst = 0.F;
		for ( Point p: from )
		{
			float best = std::numeric_limits<float>::max();
			for ( size_t i = 0; i < to.size(); ++i )
			{
				const Point a = to[i];
				const Point b = to[( i + 1 ) % to.size()];
				const Point ab = b - a;
				const float len2 = ab.lengthSquared();
				const float t = len2 > 0.F ? std::clamp( ( ( p - a ).x() * ab.x() + ( p - a ).y() * ab.y() ) / len2, 0.F, 1.F ) : 0.F;
				best = std::min( best, ( p - ( a + ab * t ) ).lengthSquared() );
			}
			worst = std::max( worst, best );
		}
		return std::sqrt( worst );
	};
	return std::max( one_way( px, py ), one_way( py, px ) );
}

// The contour of `pairs` that traces `original` best (cut contours come out in another order)
float closest( const Contour &original, const std::vector<ContourPair> &pairs, bool second )
{
	float best = std::numeric_limits<float>::max();
	for ( const auto &pair: pairs )
	{
		best = std::min( best, distance_between( original, second ? pair.b : pair.a ) );
	}
	return best;
}
}

TEST_CASE( "match_contours cuts glyphs without changing their shape", "[morph]" )
{
	auto theme = AcademicClassic::create_theme();
	const FontStyle style = FontStyle{ Intent::PROSE }.huge();

	const char *letters[] = { "a", "e", "8", "x", "g", "S" };
	for ( const char *from: letters )
	{
		for ( const char *to: letters )
		{
			auto a = to_outline( *make_text( theme, from, style ) );
			auto b = to_outline( *make_text( theme, to, style ) );
			REQUIRE( a.size() == 1 );
			REQUIRE( b.size() == 1 );
			const float size = std::hypot( bounds_of( *make_text( theme, from, style ) ).width(), bounds_of( *make_text( theme, from, style ) ).height() );

			const auto pairs = match_contours( a[0], b[0] );
			INFO( from << " -> " << to );
			for ( const auto &c: a[0].contours )
			{
				CHECK( closest( c, pairs, false ) < 1e-4F * size );
			}
			for ( const auto &c: b[0].contours )
			{
				CHECK( closest( c, pairs, true ) < 1e-4F * size );
			}
		}
	}
}

TEST_CASE( "Morph with stagger goes on smoothly past its ends (for eases that overshoot)", "[morph]" )
{
	MorphOptions options;
	options.stagger = 0.5F;
	auto a = discs();
	auto b = boxes();
	Morph morph( *a, *b, options );

	// Just past either end, nothing jumps: every part is within a hair of where it was at the end
	for ( const auto &[end, past]: { std::pair{ 1.F, 1.001F }, std::pair{ 0.F, -0.001F } } )
	{
		morph.set( end );
		std::vector<double> at_end;
		for ( size_t k = 0; k < 3; ++k )
		{
			at_end.push_back( part_area( morph, k ) );
		}
		const AABB box_end = bounds_of( morph.visual() );

		morph.set( past );
		for ( size_t k = 0; k < 3; ++k )
		{
			INFO( "part " << k << " at t = " << past );
			CHECK( part_area( morph, k ) == Approx( at_end[k] ).epsilon( 0.01 ) );
		}
		const AABB box_past = bounds_of( morph.visual() );
		CHECK( box_past.width() == Approx( box_end.width() ).epsilon( 0.01 ) );
	}
}

TEST_CASE( "Morph draws the outlines as they are at its ends, not cut to match", "[morph]" )
{
	auto theme = AcademicClassic::create_theme();
	const FontStyle style{ Intent::PROSE };
	auto a = make_text( theme, "a", style );
	auto e = make_text( theme, "e", style );
	const auto outline_a = to_outline( *a );
	const auto outline_e = to_outline( *e );

	auto segments = []( const std::vector<OutlineShape> &shapes )
	{
		size_t n = 0;
		for ( const auto &shape: shapes )
			for ( const auto &c: shape.contours )
				n += c.segs.size();
		return n;
	};

	Morph morph( *a, *e );
	morph.set( 0.F );
	CHECK( segments( to_outline( morph.visual() ) ) == segments( outline_a ) );
	morph.set( 1.F );
	CHECK( segments( to_outline( morph.visual() ) ) == segments( outline_e ) );

	// In between, both cut to the same number of pieces (more than either has)
	morph.set( 0.5F );
	CHECK( segments( to_outline( morph.visual() ) ) > std::max( segments( outline_a ), segments( outline_e ) ) );
}

TEST_CASE( "A disc collapsing into a line does it evenly, top and bottom alike", "[morph]" )
{
	// As the demo does: a disc, and a line (off centre) that it becomes, with the line on the disc's middle
	Circle disc;
	disc.radius = 0.8F;
	Line line;
	line.start = { -1.5F, 0.F };
	line.end = { 1.2F, 0.F };
	line.stroke_thickness = 0.12F;

	Morph morph( disc, line );
	for ( float t: { 0.25F, 0.5F, 0.75F } )
	{
		morph.set( t );
		// Its tips (leftmost and rightmost points) stay on the line's axis, as they would if the top and bottom
		// halves came in alike; lopsided, the shape leans and a tip rises off it
		Point left{ 1e9F, 0.F };
		Point right{ -1e9F, 0.F };
		for ( const auto &shape: to_outline( morph.visual() ) )
		{
			for ( const auto &c: shape.contours )
			{
				for ( Point p: trace( c, 64 ) )
				{
					if ( p.x() < left.x() )
						left = p;
					if ( p.x() > right.x() )
						right = p;
				}
			}
		}
		const float height = bounds_of( morph.visual() ).height();
		INFO( "t = " << t << ": left tip at y " << left.y() << ", right tip at y " << right.y() << ", height " << height );
		CHECK( std::abs( left.y() ) < 0.05F * height + 1e-4F );
		CHECK( std::abs( right.y() ) < 0.05F * height + 1e-4F );
	}
}

TEST_CASE( "A stroke morphing into a fill is bevelled in between, so its sharpening ends never snap", "[morph]" )
{
	Circle disc;
	disc.radius = 0.8F;
	Line line;
	line.start = { -1.5F, 0.F };
	line.end = { 1.2F, 0.F };
	line.stroke_thickness = 0.12F;

	auto join_at = [&]( Morph &morph, float t )
	{
		morph.set( t );
		return static_cast<const Path &>( *morph.visual().begin()->get() ).join;
	};

	Morph morph( disc, line );
	CHECK( join_at( morph, 0.3F ) == LineJoin::BEVEL );
	CHECK( join_at( morph, 0.97F ) == LineJoin::BEVEL );
	// At the ends it is drawn as the visuals themselves are
	CHECK( join_at( morph, 0.F ) == LineJoin::MITER );
	CHECK( join_at( morph, 1.F ) == LineJoin::MITER );

	// Two filled shapes keep their mitres
	Rectangle square;
	square.half_width = square.half_height = 0.5F;
	Morph fills( disc, square );
	CHECK( join_at( fills, 0.5F ) == LineJoin::MITER );
}

namespace
{
// The demo's arrow: a shaft and a head, turned `degrees` about `spot`
std::unique_ptr<Group> demo_arrow( float degrees )
{
	auto group = std::make_unique<Group>();
	auto shaft = std::make_unique<Line>();
	shaft->start = { -1.5F, 0.F };
	shaft->end = { 1.2F, 0.F };
	shaft->stroke_thickness = 0.12F;
	auto tip = std::make_unique<ArrowTip>();
	tip->start = { -1.5F, 0.F };
	tip->end = { 1.6F, 0.F };
	tip->size = 0.3F;
	group->add( std::move( shaft ) );
	group->add( std::move( tip ) );
	group->position = { 0.F, 1.F };
	group->rotation = degrees;
	return group;
}

// Corners (segment ends) of the drawn head: the second leaf of a morph of the arrow
std::vector<Point> head_corners( Morph &morph )
{
	std::vector<Point> out;
	const auto shapes = to_outline( morph.visual() );
	for ( const auto &c: shapes[1].contours )
	{
		for ( const auto &s: c.segs )
		{
			out.push_back( s.to );
		}
	}
	return out;
}
}

TEST_CASE( "A shape that is only turned is carried round whole, not squashed", "[morph]" )
{
	auto before = demo_arrow( 0.F );
	auto after = demo_arrow( 60.F );
	Morph morph( *before, *after );

	morph.set( 0.F );
	const auto start = head_corners( morph );
	REQUIRE( start.size() == 3 );

	for ( float t: { 0.25F, 0.5F, 0.75F } )
	{
		morph.set( t );
		const auto now = head_corners( morph );
		INFO( "t = " << t );
		REQUIRE( now.size() == 3 ); // still a triangle, not cut into more
		// the same triangle: its sides keep their lengths
		for ( size_t i = 0; i < 3; ++i )
		{
			const Point e0 = start[( i + 1 ) % 3] - start[i];
			const Point e1 = now[( i + 1 ) % 3] - now[i];
			CHECK( std::sqrt( e1.lengthSquared() ) == Approx( std::sqrt( e0.lengthSquared() ) ).epsilon( 1e-3 ) );
		}
		// and the head stays at the shaft's end: both turn about the arrow's pivot, together
		const auto shaft = to_outline( morph.visual() )[0].contours[0];
		const Point shaft_end = shaft.segs.front().to; // (the outline of the drawn path closes back to its start after)
		const Point spot{ 0.F, 1.F };
		const float a = std::atan2( shaft_end.y() - spot.y(), shaft_end.x() - spot.x() );
		CHECK( a == Approx( t * 60.F * std::numbers::pi_v<float> / 180.F ).margin( 1e-3 ) );
		CHECK( std::sqrt( ( shaft_end - spot ).lengthSquared() ) == Approx( 1.2F ).epsilon( 1e-3 ) ); // not shortened
	}
}

TEST_CASE( "Similarity::between finds a shift, a turn, or nothing", "[morph]" )
{
	const Contour square = rect_contour( 1.F, 1.F );
	auto moved = [&]( float dx, float degrees, float scale )
	{
		Contour c = square;
		const float r = degrees * std::numbers::pi_v<float> / 180.F;
		auto f = [&]( Point p ) { return Point{ scale * ( std::cos( r ) * p.x() - std::sin( r ) * p.y() ) + dx, scale * ( std::sin( r ) * p.x() + std::cos( r ) * p.y() ) }; };
		c.start = f( c.start );
		for ( auto &s: c.segs )
		{
			s.c1 = f( s.c1 );
			s.c2 = f( s.c2 );
			s.to = f( s.to );
		}
		return c;
	};

	auto shifted = Similarity::between( { square }, { moved( 3.F, 0.F, 1.F ) } );
	REQUIRE( shifted );
	CHECK( shifted->shift_only );
	CHECK( shifted->apply( { 1.F, 1.F }, 0.5F ).x() == Approx( 2.5F ) );

	auto turned = Similarity::between( { square }, { moved( 3.F, 90.F, 2.F ) } );
	REQUIRE( turned );
	CHECK_FALSE( turned->shift_only );
	CHECK( turned->angle == Approx( std::numbers::pi_v<float> / 2.F ) );
	const Point end = turned->apply( square.start, 1.F );
	CHECK( same_point( end, moved( 3.F, 90.F, 2.F ).start ) );

	// A different shape (squashed) is not a similarity
	CHECK_FALSE( Similarity::between( { square }, { rect_contour( 2.F, 1.F ) } ) );
}
