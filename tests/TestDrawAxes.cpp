#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Stage.h>
#include <plot/DrawAxes.h>
#include <themes/NewComputerModern.h>
#include <visuals/Line.h>
#include <visuals/Tags.h>

#include <chrono>
#include "TestThemes.h"

using Catch::Approx;
using namespace std::chrono_literals;

namespace
{
std::unique_ptr<Axes> make( Theme &theme )
{
	AxesOptions options;
	options.x = { -3.0, 3.0 };
	options.y = { -2.0, 2.0 };
	options.width = 6.F;
	options.height = 4.F;
	options.grid = true;
	return make_axes( theme, options );
}

struct Snapshot
{
	std::vector<Point> starts;
	std::vector<Point> ends;
	std::vector<float> alphas;
};

Snapshot snapshot( Axes &axes )
{
	Snapshot s;
	for ( Visual *part: find_tagged( axes, AXIS_LINE_TAG ) )
	{
		s.starts.push_back( dynamic_cast<Line *>( part )->start );
		s.ends.push_back( dynamic_cast<Line *>( part )->end );
	}
	for ( const char *tag: { TICK_TAG, NUMBER_TAG, AXIS_NAME_TAG, AXIS_ARROW_TAG } )
	{
		for ( Visual *part: find_tagged( axes, tag ) )
		{
			s.alphas.push_back( part->color.a );
			s.alphas.push_back( part->stroke_color.a );
		}
	}
	s.alphas.push_back( ( **axes.begin() ).color.a );
	return s;
}
}

TEST_CASE( "draw_axes leaves the axes exactly as they were", "[drawaxes]" )
{
	Theme theme = test_theme();
	Stage stage;
	auto axes = make( theme );
	const Snapshot before = snapshot( *axes );

	Scene scene = draw_axes( stage, *axes, 1s );
	scene.resume();
	while ( !scene.done() )
		scene.resume();

	const Snapshot after = snapshot( *axes );
	REQUIRE( after.starts.size() == before.starts.size() );
	for ( size_t i = 0; i < before.starts.size(); ++i )
	{
		CHECK( after.starts[i].x() == before.starts[i].x() );
		CHECK( after.starts[i].y() == before.starts[i].y() );
		CHECK( after.ends[i].x() == before.ends[i].x() );
		CHECK( after.ends[i].y() == before.ends[i].y() );
	}
	REQUIRE( after.alphas.size() == before.alphas.size() );
	for ( size_t i = 0; i < before.alphas.size(); ++i )
		CHECK( after.alphas[i] == Approx( before.alphas[i] ) );
}

TEST_CASE( "draw_axes starts from nothing, and the lines grow out from where they cross", "[drawaxes]" )
{
	Theme theme = test_theme();
	Stage stage;
	auto axes = make( theme );

	Scene scene = draw_axes( stage, *axes, 2s );
	scene.resume(); // the first frame
	for ( Visual *part: find_tagged( *axes, TICK_TAG ) )
		CHECK( part->stroke_color.a == 0.F );
	for ( Visual *part: find_tagged( *axes, AXIS_LINE_TAG ) )
	{
		const auto *line = dynamic_cast<Line *>( part );
		CHECK( line->start.x() == Approx( 0.F ) ); // both at the crossing (the origin here)
		CHECK( line->end.y() == Approx( 0.F ) );
	}

	// Part of the way through the lines: partly grown, the ticks not yet come, the grid not either
	for ( int i = 0; i < 12; ++i )
		scene.resume();
	bool grown = false;
	for ( Visual *part: find_tagged( *axes, AXIS_LINE_TAG ) )
	{
		const auto *line = dynamic_cast<Line *>( part );
		const float length = std::abs( line->end.x() - line->start.x() ) + std::abs( line->end.y() - line->start.y() );
		grown = grown || length > 0.5F;
		CHECK( length < 6.F ); // not yet all the way (the x axis is 6 long)
	}
	CHECK( grown );
	CHECK( ( **axes->begin() ).color.a == 0.F );

	while ( !scene.done() )
		scene.resume();
}

TEST_CASE( "draw_axes brings the ticks in from the crossing outward", "[drawaxes]" )
{
	Theme theme = test_theme();
	Stage stage;
	auto axes = make( theme );

	// The frame each tick first shows, by its distance from the crossing
	std::vector<std::pair<float, int>> first_seen;
	const auto ticks = find_tagged( *axes, TICK_TAG );
	std::vector<bool> seen( ticks.size(), false );
	Scene scene = draw_axes( stage, *axes, 3s );
	int frame = 0;
	scene.resume();
	while ( !scene.done() )
	{
		for ( size_t i = 0; i < ticks.size(); ++i )
		{
			if ( !seen[i] && ticks[i]->stroke_color.a > 0.F )
			{
				seen[i] = true;
				const auto *mark = dynamic_cast<const Line *>( ticks[i] );
				const Point middle = ( mark->start + mark->end ) / 2.F;
				first_seen.emplace_back( std::sqrt( middle.lengthSquared() ), frame );
			}
		}
		++frame;
		scene.resume();
	}
	REQUIRE( first_seen.size() == ticks.size() );
	// Nearer ones show up no later than farther ones
	for ( size_t i = 1; i < first_seen.size(); ++i )
	{
		if ( first_seen[i].first > first_seen[i - 1].first + 1e-3F )
			CHECK( first_seen[i].second >= first_seen[i - 1].second );
	}
}
