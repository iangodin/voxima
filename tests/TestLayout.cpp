#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/Bounds.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Group.h>
#include <visuals/Layout.h>

using Catch::Approx;

TEST_CASE( "layout_extent of plain shapes", "[visuals]" )
{
	Circle circle;
	circle.radius = 2.F;
	circle.position = { 1.F, 3.F };

	SECTION( "falls back to the painted bounds" )
	{
		AABB box = layout_extent( circle );
		CHECK( box.min.x() == Approx( -1.F ) );
		CHECK( box.max.x() == Approx( 3.F ) );
		CHECK( box.min.y() == Approx( 1.F ) );
		CHECK( box.max.y() == Approx( 5.F ) );
	}

	SECTION( "an explicit extent overrides the painted bounds and follows position" )
	{
		AABB custom;
		custom.add( { 0.F, 0.F } );
		custom.add( { 10.F, 1.F } );
		circle.extent = custom;

		AABB box = layout_extent( circle );
		CHECK( box.min.x() == Approx( 1.F ) );
		CHECK( box.max.x() == Approx( 11.F ) );
		CHECK( box.min.y() == Approx( 3.F ) );
		CHECK( box.max.y() == Approx( 4.F ) );
	}

	SECTION( "the extent scales with the visual" )
	{
		AABB custom;
		custom.add( { 0.F, 0.F } );
		custom.add( { 10.F, 1.F } );
		circle.extent = custom;
		circle *= 2.F;

		CHECK( circle.extent->max.x() == Approx( 20.F ) );
		CHECK( circle.extent->max.y() == Approx( 2.F ) );
	}
}

TEST_CASE( "layout_extent of groups", "[visuals]" )
{
	Group group;
	group.position = { 10.F, 0.F };
	auto &a = group.add( std::make_unique<Circle>() );
	a.position = { 0.F, 0.F };
	auto &b = group.add( std::make_unique<Circle>() );
	b.position = { 5.F, 0.F };

	AABB box = layout_extent( group );
	CHECK( box.min.x() == Approx( 9.F ) );
	CHECK( box.max.x() == Approx( 16.F ) );
}

TEST_CASE( "text layout uses advance widths, not painted width", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();
	auto text = make_text( theme, "l. ", FontStyle{ Intent::PROSE } );

	AABB layout = layout_extent( *text );
	BoundingBoxVisitor ink;
	text->accept( ink );

	// The box starts at the origin and, because of side bearings and the trailing
	// space, is wider than what is painted.
	CHECK( layout.min.x() == Approx( 0.F ) );
	CHECK( layout.width() > ink.bounds.width() );
	CHECK( layout.min.x() <= ink.bounds.min.x() );
	CHECK( layout.max.x() > ink.bounds.max.x() );
}
