#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Group.h>
#include <visuals/Morph.h>
#include <visuals/MorphMatch.h>
#include <visuals/Outline.h>
#include <visuals/Rectangle.h>

#include <cmath>
#include <numbers>

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
