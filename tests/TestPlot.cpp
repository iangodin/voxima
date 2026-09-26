#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <plot/Axes.h>
#include <plot/Graph.h>
#include <themes/NewComputerModern.h>
#include <visuals/Glyph.h>
#include <visuals/Line.h>

#include <cmath>
#include <variant>

using Catch::Approx;

TEST_CASE( "nice_step picks 1, 2 or 5 times a power of 10", "[plot]" )
{
	CHECK( nice_step( 10.0, 10.0 ) == Approx( 1.0 ) );
	CHECK( nice_step( 10.0, 5.0 ) == Approx( 2.0 ) );
	CHECK( nice_step( 100.0, 20.0 ) == Approx( 5.0 ) );
	CHECK( nice_step( 1.0, 10.0 ) == Approx( 0.1 ) );
	CHECK( nice_step( 0.03, 3.0 ) == Approx( 0.01 ) );
	CHECK( nice_step( 700.0, 10.0 ) == Approx( 50.0 ) );
}

TEST_CASE( "tick_values are the multiples of the step in the range", "[plot]" )
{
	const auto v = tick_values( { -2.5, 3.2 }, 1.0 );
	REQUIRE( v.size() == 6 );
	CHECK( v.front() == -2.0 );
	CHECK( v.back() == 3.0 );
	CHECK( v[2] == 0.0 );

	const auto tenths = tick_values( { 0.0, 0.3 }, 0.1 );
	CHECK( tenths.size() == 4 ); // 0, 0.1, 0.2, 0.3: the end is not lost to rounding
}

TEST_CASE( "tick_text has the step's decimals and a real minus sign", "[plot]" )
{
	CHECK( tick_text( 2.0, 1.0 ) == "2" );
	CHECK( tick_text( -3.0, 1.0 ) == "−3" );
	CHECK( tick_text( 0.5, 0.5 ) == "0.5" );
	CHECK( tick_text( -0.25, 0.05 ) == "−0.25" );
	CHECK( tick_text( 1500.0, 500.0 ) == "1500" );
	CHECK( tick_text( -1e-12, 0.1 ) == "0.0" ); // not "-0.0"
}

TEST_CASE( "Axes put data points on the plot area", "[plot]" )
{
	AxesOptions options;
	options.x = { -4.0, 4.0 };
	options.y = { 0.0, 10.0 };
	options.width = 8.F;
	options.height = 5.F;
	Axes axes( options );

	CHECK( axes.at( -4.0, 0.0 ).x() == Approx( -4.F ) );
	CHECK( axes.at( -4.0, 0.0 ).y() == Approx( -2.5F ) );
	CHECK( axes.at( 4.0, 10.0 ).x() == Approx( 4.F ) );
	CHECK( axes.at( 4.0, 10.0 ).y() == Approx( 2.5F ) );
	CHECK( axes.at( 0.0, 5.0 ).x() == Approx( 0.F ).margin( 1e-6 ) );

	// 0 is in x's range, so the y axis is there; y's range starts at 0
	CHECK( axes.x_crossing() == 0.0 );
	CHECK( axes.y_crossing() == 0.0 );

	options.y = { 2.0, 10.0 };
	CHECK( Axes( options ).y_crossing() == 2.0 ); // 0 not in range: along the bottom
}

TEST_CASE( "make_axes ticks every value but the origin and the one under an arrow", "[plot]" )
{
	Theme theme = NewComputerModern::create_theme();
	AxesOptions options;
	options.x = { -3.0, 3.0 };
	options.y = { -2.0, 2.0 };
	options.width = 6.F;
	options.height = 4.F;
	options.numbers = false;
	options.x_label = "";
	options.y_label = "";
	auto axes = make_axes( theme, options );

	CHECK( axes->x_ticks().size() == 7 ); // -3 .. 3
	CHECK( axes->y_ticks().size() == 5 ); // -2 .. 2

	// Lines, in the layer of the axes: two axes, then a tick for each value except 0 (the origin) and 3 and 2
	// (under the arrow tips)
	size_t lines = 0;
	const auto &layer = dynamic_cast<const Group &>( **( axes->begin() + 1 ) );
	for ( const auto &child: layer )
	{
		if ( dynamic_cast<const Line *>( child.get() ) )
			++lines;
	}
	CHECK( lines == 2 + ( 7 - 2 ) + ( 5 - 2 ) );
}

namespace
{
Axes demo_axes()
{
	AxesOptions options;
	options.x = { -6.0, 6.0 };
	options.y = { -2.5, 2.5 };
	options.width = 12.F;
	options.height = 5.F;
	return Axes( options );
}

// Where the drawn path ends
Point last_point( const FunctionGraph &graph )
{
	const auto &c = graph.commands.back();
	if ( const auto *cubic = std::get_if<CubicTo>( &c ) )
		return cubic->to;
	return std::get<MoveTo>( c ).to;
}
}

TEST_CASE( "plot draws a function as one piece through its points", "[plot]" )
{
	const Axes axes = demo_axes();
	auto wave = plot( axes, []( double x ) { return std::sin( x ); } );
	REQUIRE( wave->pieces().size() == 1 );
	const auto &points = wave->pieces()[0];
	CHECK( points.front().x() == Approx( axes.at( -6.0, 0.0 ).x() ) );
	CHECK( points.back().x() == Approx( axes.at( 6.0, 0.0 ).x() ) );
	for ( Point p: points )
	{
		// every point is on the curve: y = sin( x ) back in data units
		const double x = -6.0 + ( p.x() - axes.left() ) / 12.0 * 12.0;
		const double y = -2.5 + ( p.y() - axes.bottom() ) / 5.0 * 5.0;
		CHECK( y == Approx( std::sin( x ) ).margin( 1e-4 ) );
	}
	CHECK( wave->stroke_thickness > 0.F );
	CHECK( wave->color.a == 0.F ); // a line, not filled
}

TEST_CASE( "plot breaks where the function leaves the plot, and stops at its edge", "[plot]" )
{
	const Axes axes = demo_axes();

	// 1/x: two pieces, each ending at the top or bottom edge by 0
	auto inverse = plot( axes, []( double x ) { return 1.0 / x; } );
	REQUIRE( inverse->pieces().size() == 2 );
	CHECK( inverse->pieces()[0].back().y() == Approx( axes.bottom() ) );
	CHECK( inverse->pieces()[1].front().y() == Approx( axes.top() ) );

	// tan: a piece between each pair of asymptotes, none drawn across them
	auto tangent = plot( axes, []( double x ) { return std::tan( x ); } );
	CHECK( tangent->pieces().size() == 5 ); // asymptotes at +-pi/2 and +-3pi/2 in -6 .. 6: five pieces between them

	// Nothing where it is not a number
	auto root = plot( axes, []( double x ) { return std::sqrt( x ); } );
	REQUIRE( root->pieces().size() == 1 );
	CHECK( root->pieces()[0].front().x() >= axes.at( 0.0, 0.0 ).x() - 1e-4F );
}

TEST_CASE( "A graph can be drawn part of the way along", "[plot]" )
{
	const Axes axes = demo_axes();
	auto line = plot( axes, []( double x ) { return x / 4.0; } );

	line->set_drawn( 0.F );
	CHECK( line->commands.empty() );

	// Half its length: a straight line, so half way across
	line->set_drawn( 0.5F );
	CHECK( last_point( *line ).x() == Approx( 0.F ).margin( 1e-3 ) );
	CHECK( last_point( *line ).y() == Approx( 0.F ).margin( 1e-3 ) );

	line->set_drawn( 1.F );
	CHECK( last_point( *line ).x() == Approx( axes.at( 6.0, 1.5 ).x() ) );
	CHECK( last_point( *line ).y() == Approx( axes.at( 6.0, 1.5 ).y() ) );
}

TEST_CASE( "Layers, bottom up: grid, axes, plots, then numbers", "[plot]" )
{
	Theme theme = NewComputerModern::create_theme();
	AxesOptions options;
	options.grid = true;
	auto axes = make_axes( theme, options );
	const auto &grid = dynamic_cast<const Group &>( **axes->begin() );
	const auto &lines = dynamic_cast<const Group &>( **( axes->begin() + 1 ) );
	const auto &plots = dynamic_cast<const Group &>( **( axes->begin() + 2 ) );
	CHECK( grid.size() > 0 );
	CHECK( lines.size() > 0 );
	CHECK( plots.size() == 0 );

	auto &wave = axes->add_plot( plot( *axes, []( double x ) { return std::sin( x ); } ) );
	REQUIRE( plots.size() == 1 );
	CHECK( ( *plots.begin() ).get() == &wave );

	// The numbers and names come after all three layers, so they are drawn over the curve
	CHECK( axes->size() > 3 );
}

TEST_CASE( "A number has a halo under it, in the color asked for, and none by default", "[plot]" )
{
	Theme theme = NewComputerModern::create_theme();
	AxesOptions options;
	options.x = { -2.0, 2.0 };
	options.y = { -1.0, 1.0 };
	options.x_label = "";
	options.y_label = "";
	auto without = make_axes( theme, options ); // (none by default)
	options.number_halo = Color{ 0.F, 0.F, 0.F, 1.F };
	auto with = make_axes( theme, options );

	// Each number is there twice: its halo, then itself
	const size_t numbers = without->size();
	CHECK( with->size() > numbers );

	// The halo is outlined, the number is not
	auto outlined = []( const Visual &v )
	{
		bool any = false;
		auto walk = [&]( auto &self, const Visual &x ) -> void
		{
			if ( const auto *g = dynamic_cast<const Glyph *>( &x ) )
				any = any || g->stroke_thickness > 0.F;
			else if ( const auto *group = dynamic_cast<const Group *>( &x ) )
				for ( const auto &c: *group )
					self( self, *c );
		};
		walk( walk, v );
		return any;
	};
	const Visual &last = **( with->begin() + static_cast<std::ptrdiff_t>( with->size() - 1 ) );
	const Visual &before = **( with->begin() + static_cast<std::ptrdiff_t>( with->size() - 2 ) );
	CHECK( outlined( before ) );
	CHECK_FALSE( outlined( last ) );
}

TEST_CASE( "Axes along the edges are at the low ends, and keep every tick", "[plot]" )
{
	Theme theme = NewComputerModern::create_theme();
	AxesOptions options;
	options.x = { -3.0, 3.0 };
	options.y = { -2.0, 2.0 };
	options.width = 6.F;
	options.height = 4.F;
	options.numbers = false;
	options.x_label = "";
	options.y_label = "";
	options.placement = AxisPlacement::EDGE;
	auto axes = make_axes( theme, options );

	// The y axis is at the left, the x axis at the bottom, though 0 is in range
	CHECK( axes->x_crossing() == -3.0 );
	CHECK( axes->y_crossing() == -2.0 );

	size_t lines = 0;
	Point x_axis_from;
	const auto &layer = dynamic_cast<const Group &>( **( axes->begin() + 1 ) );
	for ( const auto &child: layer )
	{
		if ( const auto *line = dynamic_cast<const Line *>( child.get() ) )
		{
			if ( lines == 0 )
				x_axis_from = line->start;
			++lines;
		}
	}
	CHECK( x_axis_from.y() == Approx( axes->bottom() ) );
	// Two axes, and every tick but the ones under the arrow tips (3 and 2): the origin's and the corner's too
	CHECK( lines == 2 + ( 7 - 1 ) + ( 5 - 1 ) );

	// In the middle (the default), the origin is not ticked, and the axes are at 0
	options.placement = AxisPlacement::ORIGIN;
	CHECK( Axes( options ).x_crossing() == 0.0 );
	CHECK( Axes( options ).y_crossing() == 0.0 );
}

TEST_CASE( "The grid is one stroked path, so its lines do not blend twice where they cross", "[plot]" )
{
	Theme theme = NewComputerModern::create_theme();
	AxesOptions options;
	options.x = { -2.0, 2.0 };
	options.y = { -1.0, 1.0 };
	options.x_step = 1.0;
	options.y_step = 1.0;
	options.grid = true;
	auto axes = make_axes( theme, options );

	const auto &grid = dynamic_cast<const Group &>( **axes->begin() );
	REQUIRE( grid.size() == 1 ); // not a line for each tick: see-through lines would show a bright spot at every crossing
	const auto *path = dynamic_cast<const Path *>( ( *grid.begin() ).get() );
	REQUIRE( path );
	CHECK( path->stroke_color.a < 1.F );
	CHECK( path->color.a == 0.F ); // lines, not a filled shape

	// A line for each tick: 5 across (-2 .. 2) and 3 up (-1 .. 1), each a move and a line
	size_t moves = 0;
	size_t lines = 0;
	for ( const auto &command: path->commands )
	{
		moves += std::holds_alternative<MoveTo>( command ) ? 1 : 0;
		lines += std::holds_alternative<LineTo>( command ) ? 1 : 0;
	}
	CHECK( moves == 5 + 3 );
	CHECK( lines == 5 + 3 );
}
