#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Ellipse.h>
#include <visuals/Group.h>
#include <visuals/Line.h>
#include <visuals/Outline.h>
#include <visuals/Path.h>
#include <visuals/Rectangle.h>
#include <visuals/RoundRectangle.h>

#include <cmath>
#include <numbers>

using Catch::Approx;

namespace
{
AABB bounds_of( const std::vector<OutlineShape> &shapes )
{
	AABB box;
	for ( const auto &shape: shapes )
	{
		for ( const auto &contour: shape.contours )
		{
			box.add( on_curve_bounds( contour ) );
		}
	}
	return box;
}

size_t contour_count( const std::vector<OutlineShape> &shapes )
{
	size_t n = 0;
	for ( const auto &shape: shapes )
	{
		n += shape.contours.size();
	}
	return n;
}
}

TEST_CASE( "to_outline flattens a circle to four cubics", "[outline]" )
{
	Circle circle;
	circle.radius = 2.F;
	circle.position = { 3.F, 1.F };

	auto shapes = to_outline( circle );
	REQUIRE( shapes.size() == 1 );
	REQUIRE( shapes[0].contours.size() == 1 );
	CHECK( shapes[0].contours[0].segs.size() == 4 );

	const AABB box = bounds_of( shapes );
	CHECK( box.min.x() == Approx( 1.F ) );
	CHECK( box.max.x() == Approx( 5.F ) );
	CHECK( box.min.y() == Approx( -1.F ) );
	CHECK( box.max.y() == Approx( 3.F ) );

	const double pi_r2 = std::numbers::pi * 4.0;
	CHECK( std::abs( signed_area( shapes[0].contours[0] ) ) == Approx( pi_r2 ).epsilon( 0.001 ) );
}

TEST_CASE( "to_outline handles ellipses, rectangles and rounded rectangles", "[outline]" )
{
	SECTION( "ellipse" )
	{
		Ellipse e;
		e.half_width = 3.F;
		e.half_height = 1.F;
		auto shapes = to_outline( e );
		const AABB box = bounds_of( shapes );
		CHECK( box.width() == Approx( 6.F ) );
		CHECK( box.height() == Approx( 2.F ) );
		CHECK( std::abs( signed_area( shapes[0].contours[0] ) ) ==
			   Approx( std::numbers::pi * 3.0 ).epsilon( 0.001 ) );
	}

	SECTION( "rectangle" )
	{
		Rectangle r;
		r.half_width = 2.F;
		r.half_height = 1.F;
		auto shapes = to_outline( r );
		REQUIRE( shapes[0].contours.size() == 1 );
		CHECK( shapes[0].contours[0].segs.size() == 4 );
		CHECK( std::abs( signed_area( shapes[0].contours[0] ) ) == Approx( 8.0 ) );
	}

	SECTION( "rounded rectangle is smaller than the plain one by the corner cut-offs" )
	{
		RoundRectangle r;
		r.half_width = 2.F;
		r.half_height = 1.F;
		r.top_left_radius = r.top_right_radius = r.bottom_left_radius = r.bottom_right_radius = 0.5F;
		auto shapes = to_outline( r );
		REQUIRE( shapes[0].contours.size() == 1 );
		// 4 sides + 4 corner arcs
		CHECK( shapes[0].contours[0].segs.size() == 8 );
		const double cut = 4.0 * ( 0.25 - std::numbers::pi * 0.25 / 4.0 );
		CHECK( std::abs( signed_area( shapes[0].contours[0] ) ) == Approx( 8.0 - cut ).epsilon( 0.002 ) );
		CHECK( bounds_of( shapes ).width() == Approx( 4.F ) );
	}

	SECTION( "zero radii give the plain rectangle" )
	{
		RoundRectangle r;
		r.half_width = 2.F;
		r.half_height = 1.F;
		auto shapes = to_outline( r );
		CHECK( shapes[0].contours[0].segs.size() == 4 );
	}
}

TEST_CASE( "to_outline flattens paths", "[outline]" )
{
	Path path;
	path.commands.push_back( MoveTo{ { 0.F, 0.F } } );
	path.commands.push_back( LineTo{ { 2.F, 0.F } } );
	path.commands.push_back( QuadTo{ { 2.F, 2.F }, { 1.F, 2.F } } ); // to, ctrl1 -> curve ends at (2,2)
	path.commands.push_back( Close{} );

	auto shapes = to_outline( path );
	REQUIRE( shapes.size() == 1 );
	REQUIRE( shapes[0].contours.size() == 1 );
	const auto &contour = shapes[0].contours[0];
	// line, quad (as cubic), and the segment that closes the loop
	CHECK( contour.segs.size() == 3 );
	CHECK( contour.segs.back().to == contour.start );

	SECTION( "a quadratic keeps its shape when raised to a cubic" )
	{
		// Quad from (2,0) with control (1,2) [ctrl1] to (2,2): midpoint is 0.25 p0 + 0.5 c + 0.25 p2
		const auto &seg = contour.segs[1];
		const Point p0 = contour.segs[0].to;
		const Point mid = p0 * 0.125F + seg.c1 * 0.375F + seg.c2 * 0.375F + seg.to * 0.125F;
		const Point expected = p0 * 0.25F + Point{ 1.F, 2.F } * 0.5F + Point{ 2.F, 2.F } * 0.25F;
		CHECK( mid.x() == Approx( expected.x() ) );
		CHECK( mid.y() == Approx( expected.y() ) );
	}

	SECTION( "each MoveTo starts a new contour" )
	{
		Path two;
		two.commands.push_back( MoveTo{ { 0.F, 0.F } } );
		two.commands.push_back( LineTo{ { 1.F, 0.F } } );
		two.commands.push_back( LineTo{ { 1.F, 1.F } } );
		two.commands.push_back( Close{} );
		two.commands.push_back( MoveTo{ { 5.F, 5.F } } );
		two.commands.push_back( LineTo{ { 6.F, 5.F } } );
		two.commands.push_back( LineTo{ { 6.F, 6.F } } );
		two.commands.push_back( Close{} );
		CHECK( to_outline( two )[0].contours.size() == 2 );
	}
}

TEST_CASE( "to_outline bakes in transforms in SVG order", "[outline]" )
{
	Rectangle rect;
	rect.half_width = 2.F;
	rect.half_height = 1.F;

	SECTION( "rotation by 90 degrees swaps width and height" )
	{
		rect.rotation = 90.F;
		const AABB box = bounds_of( to_outline( rect ) );
		CHECK( box.width() == Approx( 2.F ).margin( 1e-4 ) );
		CHECK( box.height() == Approx( 4.F ).margin( 1e-4 ) );
	}

	SECTION( "scale happens before rotation and translation" )
	{
		rect.scale_x = 2.F;
		rect.position = { 10.F, 0.F };
		const AABB box = bounds_of( to_outline( rect ) );
		CHECK( box.min.x() == Approx( 6.F ) );
		CHECK( box.max.x() == Approx( 14.F ) );
		CHECK( box.height() == Approx( 2.F ) );
	}

	SECTION( "group transforms compose with the leaf's own" )
	{
		Group group;
		group.position = { 1.F, 0.F };
		group.scale_x = group.scale_y = 2.F;
		auto leaf = std::make_unique<Rectangle>();
		leaf->half_width = 1.F;
		leaf->half_height = 1.F;
		leaf->position = { 3.F, 0.F };
		group.add( std::move( leaf ) );

		const AABB box = bounds_of( to_outline( group ) );
		// leaf spans x 2..4 in the group; the group doubles it and moves it by 1
		CHECK( box.min.x() == Approx( 5.F ) );
		CHECK( box.max.x() == Approx( 9.F ) );
		CHECK( box.height() == Approx( 4.F ) );
	}

	SECTION( "nested groups and leaf rotation" )
	{
		Group outer;
		outer.rotation = 90.F;
		auto inner = std::make_unique<Group>();
		inner->position = { 1.F, 0.F };
		auto leaf = std::make_unique<Rectangle>();
		leaf->half_width = 1.F;
		leaf->half_height = 1.F;
		inner->add( std::move( leaf ) );
		outer.add( std::move( inner ) );

		// the unit square around (1,0), rotated a quarter turn, is centred on (0,1)
		const AABB box = bounds_of( to_outline( outer ) );
		CHECK( box.min.x() == Approx( -1.F ).margin( 1e-4 ) );
		CHECK( box.max.x() == Approx( 1.F ).margin( 1e-4 ) );
		CHECK( box.min.y() == Approx( 0.F ).margin( 1e-4 ) );
		CHECK( box.max.y() == Approx( 2.F ).margin( 1e-4 ) );
	}
}

TEST_CASE( "to_outline carries colors, one shape per leaf", "[outline]" )
{
	Group group;
	group.color = { 1.F, 0.5F, 0.F, 1.F };
	for ( int i = 0; i < 2; ++i )
	{
		auto leaf = std::make_unique<Circle>();
		leaf->color = { 1.F, 1.F, 1.F, i == 0 ? 1.F : 0.5F };
		leaf->stroke_thickness = 0.1F;
		group.add( std::move( leaf ) );
	}
	group.scale_x = group.scale_y = 2.F;

	auto shapes = to_outline( group );
	REQUIRE( shapes.size() == 2 );
	CHECK( shapes[0].fill == Color{ 1.F, 0.5F, 0.F, 1.F } );
	CHECK( shapes[1].fill == Color{ 1.F, 0.5F, 0.F, 0.5F } );
	// widths scale with the transform, since the outline is already in the parent's frame
	CHECK( shapes[0].stroke_thickness == Approx( 0.2F ) );
}

TEST_CASE( "to_outline skips what has no filled outline", "[outline]" )
{
	Group group;
	group.add( std::make_unique<Line>() );
	group.add( std::make_unique<Circle>() );

	size_t skipped = 0;
	auto shapes = to_outline( group, &skipped );
	CHECK( shapes.size() == 1 );
	CHECK( skipped == 1 );
}

TEST_CASE( "reversed walks a contour the other way", "[outline]" )
{
	Circle circle;
	auto shapes = to_outline( circle );
	const Contour &c = shapes[0].contours[0];
	const Contour r = reversed( c );

	REQUIRE( r.segs.size() == c.segs.size() );
	CHECK( r.start == c.start );
	CHECK( signed_area( r ) == Approx( -signed_area( c ) ) );
	CHECK( r.segs.back().to == r.start );
	CHECK( reversed( r ).segs.back().to == c.segs.back().to );
	CHECK( signed_area( reversed( r ) ) == Approx( signed_area( c ) ) );
}

TEST_CASE( "to_outline gives a glyph with a hole two opposite contours", "[outline]" )
{
	auto theme = AcademicClassic::create_theme();

	SECTION( "o has an outer and a hole" )
	{
		auto text = make_text( theme, "o", FontStyle{ Intent::PROSE } );
		auto shapes = to_outline( *text );
		REQUIRE( shapes.size() == 1 );
		REQUIRE( shapes[0].contours.size() == 2 );
		const double a = signed_area( shapes[0].contours[0] );
		const double b = signed_area( shapes[0].contours[1] );
		CHECK( a * b < 0.0 ); // opposite windings
	}

	SECTION( "x is a single contour" )
	{
		auto text = make_text( theme, "x", FontStyle{ Intent::PROSE } );
		CHECK( contour_count( to_outline( *text ) ) == 1 );
	}

	SECTION( "a word gives one shape per letter, laid out left to right" )
	{
		auto text = make_text( theme, "ab", FontStyle{ Intent::PROSE } );
		auto shapes = to_outline( *text );
		REQUIRE( shapes.size() == 2 );
		CHECK( on_curve_bounds( shapes[1].contours[0] ).min.x() > on_curve_bounds( shapes[0].contours[0] ).min.x() );
	}
}
