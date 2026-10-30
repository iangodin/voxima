#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Stage.h>
#include <visuals/Connector.h>
#include <visuals/Rectangle.h>

using Catch::Approx;

namespace
{
Rectangle box( float x, float y )
{
	Rectangle r;
	r.half_width = 1.F;
	r.half_height = 0.5F;
	r.position = { x, y };
	return r;
}
}

TEST_CASE( "A connector runs from the edge of one box to the edge of the other", "[connector]" )
{
	Rectangle a = box( 0.F, 0.F );
	Rectangle b = box( 6.F, 0.F );
	Connector c( a, b, { .gap = 0.F, .arrow = false } );
	auto &line = c.line();
	CHECK( line.start.x() == Approx( 1.F ) );
	CHECK( line.end.x() == Approx( 5.F ) );
	CHECK( line.start.y() == Approx( 0.F ) );
}

TEST_CASE( "A connector's arrow tip ends at the box, the line at its base", "[connector]" )
{
	Rectangle a = box( 0.F, 0.F );
	Rectangle b = box( 6.F, 0.F );
	Connector c( a, b, { .gap = 0.1F, .arrow_size = 0.2F } );
	auto &line = c.line();
	auto &tip = c.end_tip();
	CHECK( tip.end.x() == Approx( 4.9F ) );
	CHECK( line.end.x() == Approx( 4.5F ) );
}

TEST_CASE( "A followed connector keeps up as things move", "[connector]" )
{
	Stage stage;
	auto &a = stage.visuals.add( std::make_unique<Rectangle>( box( 0.F, 0.F ) ) );
	auto &b = stage.visuals.add( std::make_unique<Rectangle>( box( 6.F, 0.F ) ) );
	auto &c = stage.visuals.add( make_connector( a, b, { .gap = 0.F, .arrow = false } ) );
	auto tie = follow( stage, c );
	b.position = { 0.F, 6.F };
	stage.updaters.run();
	auto &line = c.line();
	CHECK( line.end.y() == Approx( 5.5F ) );
	CHECK( line.end.x() == Approx( 0.F ) );
}
