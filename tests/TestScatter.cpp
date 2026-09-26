#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <plot/Scatter.h>
#include <themes/NewComputerModern.h>
#include <visuals/Circle.h>
#include <visuals/Path.h>
#include <visuals/Placement.h>

using Catch::Approx;

namespace
{
std::unique_ptr<Axes> small_axes( Theme &theme )
{
	AxesOptions options;
	options.x = { 0.0, 10.0 };
	options.y = { 0.0, 5.0 };
	options.width = 10.F;
	options.height = 5.F;
	options.numbers = false;
	options.x_label = "";
	options.y_label = "";
	return make_axes( theme, options );
}
}

TEST_CASE( "a scatter has a marker for each point, where the point is", "[scatter]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto axes = small_axes( theme );
	auto points = scatter( *axes, { { 2.0, 1.0 }, { 5.0, 4.0 }, { 8.0, 2.5 } } );

	REQUIRE( points->count() == 3 );
	CHECK( points->line() == nullptr );
	for ( size_t i = 0; i < 3; ++i )
	{
		CHECK( dynamic_cast<const Circle *>( &points->marker( i ) ) != nullptr );
	}
	CHECK( points->marker( 1 ).position.x() == Approx( axes->at( 5.0, 4.0 ).x() ) );
	CHECK( points->marker( 1 ).position.y() == Approx( axes->at( 5.0, 4.0 ).y() ) );
}

TEST_CASE( "a point outside the plot is skipped but keeps its place, and breaks the line", "[scatter]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto axes = small_axes( theme );
	auto points = scatter( *axes, { { 1.0, 1.0 }, { 2.0, 2.0 }, { 50.0, 1.0 }, { 4.0, 3.0 }, { 5.0, 4.0 } }, { .line = JoinStyle::STRAIGHT } );

	REQUIRE( points->count() == 5 );
	CHECK( dynamic_cast<const Group *>( &points->marker( 2 ) ) != nullptr ); // nothing to draw
	CHECK( box_of( points->marker( 2 ) ).empty() );

	const auto *line = dynamic_cast<const Path *>( points->line() );
	REQUIRE( line );
	int moves = 0;
	for ( const PathCommand &command: line->commands )
		moves += std::holds_alternative<MoveTo>( command ) ? 1 : 0;
	CHECK( moves == 2 ); // two runs
}

TEST_CASE( "scatter markers of every kind, and lines of every style", "[scatter]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto axes = small_axes( theme );
	for ( MarkerShape kind: { MarkerShape::CIRCLE, MarkerShape::SQUARE, MarkerShape::DIAMOND, MarkerShape::TRIANGLE, MarkerShape::CROSS, MarkerShape::PLUS } )
	{
		auto points = scatter( *axes, { { 3.0, 3.0 } }, { .marker = kind } );
		const AABB box = box_of( points->marker( 0 ) );
		CHECK( !box.empty() );
		// About the marker's size, around the point
		CHECK( ( box.min.x() + box.max.x() ) / 2.F == Approx( axes->at( 3.0, 3.0 ).x() ).margin( 0.05 ) );
	}
	for ( JoinStyle style: { JoinStyle::STRAIGHT, JoinStyle::SMOOTH } )
	{
		auto points = scatter( *axes, { { 1.0, 1.0 }, { 3.0, 4.0 }, { 5.0, 2.0 } }, { .line = style } );
		const auto *line = dynamic_cast<const Path *>( points->line() );
		REQUIRE( line );
		CHECK( line->commands.size() == 3 );
		CHECK( std::holds_alternative<CubicTo>( line->commands[1] ) == ( style == JoinStyle::SMOOTH ) );
	}
}

TEST_CASE( "scatter series take the axes' colors in turn, or the one given", "[scatter]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto axes = small_axes( theme );
	const Color first = scatter( *axes, { { 1.0, 1.0 } } )->color();
	const Color second = scatter( *axes, { { 1.0, 1.0 } } )->color();
	CHECK( ( first.r != second.r || first.g != second.g || first.b != second.b ) );
	CHECK( scatter( *axes, { { 1.0, 1.0 } }, { .color = Color{ 1.F, 0.F, 1.F, 1.F } } )->color().g == 0.F );
}
