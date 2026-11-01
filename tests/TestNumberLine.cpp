#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <plot/NumberLine.h>
#include <themes/NewComputerModern.h>
#include <visuals/Placement.h>
#include "TestThemes.h"

using Catch::Approx;

TEST_CASE( "a number line puts values along its length, the middle of the range at the origin", "[numberline]" )
{
	Theme theme = test_theme();
	auto line = make_number_line( theme, { .range = { 0.0, 10.0 }, .length = 10.F } );
	CHECK( line->at( 5.0 ).x() == Approx( 0.F ) );
	CHECK( line->at( 0.0 ).x() == Approx( -5.F ) );
	CHECK( line->at( 10.0 ).x() == Approx( 5.F ) );
	CHECK( line->at( 2.5 ).y() == Approx( 0.F ) );
}

TEST_CASE( "marks and intervals sit on their values", "[numberline]" )
{
	Theme theme = test_theme();
	auto line = make_number_line( theme, { .range = { 0.0, 10.0 }, .length = 10.F } );

	Group &mark = line->add_mark( theme, 7.0, "x", Color{ 1.F, 0.F, 0.F, 1.F } );
	const AABB mark_box = box_of( mark );
	CHECK( ( mark_box.min.x() + mark_box.max.x() ) / 2.F == Approx( line->at( 7.0 ).x() ).margin( 0.05 ) );
	CHECK( mark_box.max.y() > 0.3F ); // the label above the dot

	Group &interval = line->add_interval( theme, 2.0, 4.0, Color{ 0.F, 0.F, 1.F, 1.F }, { .closed_to = false } );
	const AABB interval_box = box_of( interval );
	CHECK( interval_box.min.x() < line->at( 2.0 ).x() ); // (the end dots reach past it)
	CHECK( interval_box.max.x() > line->at( 4.0 ).x() );
	CHECK( interval_box.max.x() < line->at( 4.5 ).x() );
}
