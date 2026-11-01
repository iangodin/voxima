#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <themes/NewComputerModern.h>
#include <visuals/Geometry.h>
#include <visuals/Placement.h>

#include <variant>
#include "TestThemes.h"

using Catch::Approx;

TEST_CASE( "angle_between measures the smaller angle in degrees", "[geometry]" )
{
	CHECK( angle_between( { 0.F, 0.F }, { 1.F, 0.F }, { 0.F, 2.F } ) == Approx( 90.F ) );
	CHECK( angle_between( { 0.F, 0.F }, { 1.F, 0.F }, { 1.F, 1.F } ) == Approx( 45.F ) );
	CHECK( angle_between( { 0.F, 0.F }, { 1.F, 0.F }, { -3.F, 0.F } ) == Approx( 180.F ) );
	CHECK( angle_between( { 1.F, 1.F }, { 2.F, 1.F }, { 1.F, 0.F } ) == Approx( 90.F ) );
}

TEST_CASE( "a polygon is closed and its labels stand outside its corners", "[geometry]" )
{
	Theme theme = test_theme();
	auto triangle = make_polygon( theme, { { -2.F, -1.F }, { 2.F, -1.F }, { 0.F, 2.F } }, { .labels = { "A", "B", "C" } } );
	REQUIRE( triangle->size() == 4 ); // the outline, three labels

	const auto &outline = static_cast<const Path &>( **triangle->begin() );
	CHECK( std::holds_alternative<Close>( outline.commands.back() ) );
	CHECK( outline.commands.size() == 4 ); // move, two lines, close

	// C is above the top corner, A below-left of its corner
	const AABB c = box_of( **( triangle->begin() + 3 ) );
	CHECK( c.min.y() > 2.F );
	const AABB a = box_of( **( triangle->begin() + 1 ) );
	CHECK( a.max.x() < -2.F );
	CHECK( a.max.y() < -1.F + 0.3F );
}

TEST_CASE( "an angle is an arc, or a square for a right angle, with its label past it", "[geometry]" )
{
	Theme theme = test_theme();

	auto arc = make_angle( theme, { 0.F, 0.F }, { 3.F, 0.F }, { 3.F, 3.F }, { .radius = 1.F, .label = "θ" } );
	REQUIRE( arc->size() == 2 );
	const auto &path = static_cast<const Path &>( **arc->begin() );
	// The arc ends on the two rays, a radius from the vertex
	CHECK( std::get<MoveTo>( path.commands.front() ).to.x() == Approx( 1.F ) );
	const Point end = std::get<CubicTo>( path.commands.back() ).to;
	CHECK( end.x() == Approx( std::cos( 3.14159265F / 4.F ) ).margin( 1e-3 ) );
	CHECK( end.y() == Approx( std::sin( 3.14159265F / 4.F ) ).margin( 1e-3 ) );
	// The label is outside the arc, on the line halving the angle
	const AABB label = box_of( **( arc->begin() + 1 ) );
	CHECK( ( label.min.x() + label.max.x() ) / 2.F > 1.F );

	auto square = make_angle( theme, { 0.F, 0.F }, { 3.F, 0.F }, { 0.F, 3.F }, { .radius = 1.F } );
	const auto &corner = static_cast<const Path &>( **square->begin() );
	CHECK( corner.commands.size() == 3 );
	CHECK( std::holds_alternative<LineTo>( corner.commands[1] ) );
}
