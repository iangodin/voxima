#include "plot/DrawAxes.h"

#include <core/Tween.h>
#include <visuals/ArrowTip.h>
#include <visuals/Line.h>
#include <visuals/Tags.h>

#include <algorithm>
#include <cmath>

namespace
{

// Show a visual at some fraction of its own alpha (its fill and its stroke: a group's multiply down into what is in it)
struct Fader
{
	Visual *visual;
	float color;
	float stroke;

	explicit Fader( Visual &v ) :
		visual( &v ),
		color( v.color.a ),
		stroke( v.stroke_color.a )
	{
	}

	void set( float amount ) const
	{
		amount = std::clamp( amount, 0.F, 1.F );
		visual->color.a = color * amount;
		visual->stroke_color.a = stroke * amount;
	}
};

// An axis line, growing out from the crossing
struct Growing
{
	Line *line;
	Point start;
	Point end;
	Point cross;
};

Point lerp( Point a, Point b, float t )
{
	return a + ( b - a ) * t;
}

// A tick mark and its number (and halo): shown together, when it is their turn
struct Tick
{
	std::vector<Fader> parts;
	float distance; // from the crossing
};

// How far through a window of the time we are, 0 to 1
float within( float t, float from, float to )
{
	return to > from ? std::clamp( ( t - from ) / ( to - from ), 0.F, 1.F ) : ( t >= to ? 1.F : 0.F );
}

}

Scene draw_axes( const Stage &stage, Axes &axes, std::chrono::duration<double> duration, const DrawAxesOptions &options )
{
	const Point cross = axes.at( axes.x_crossing(), axes.y_crossing() );

	// The parts, found by their tags
	std::vector<Growing> lines;
	for ( Visual *part: find_tagged( axes, AXIS_LINE_TAG ) )
	{
		auto *line = dynamic_cast<Line *>( part );
		lines.push_back( { line, line->start, line->end, cross } );
	}
	std::vector<Fader> arrows;
	for ( Visual *part: find_tagged( axes, AXIS_ARROW_TAG ) )
	{
		arrows.emplace_back( *part );
	}
	std::vector<Fader> names;
	for ( Visual *part: find_tagged( axes, AXIS_NAME_TAG ) )
	{
		names.emplace_back( *part );
	}
	std::vector<Tick> ticks;
	for ( int n = 0;; ++n )
	{
		const auto found = find_tagged( axes, TICK_TAG + std::string( "-" ) + std::to_string( n ) );
		if ( found.empty() )
		{
			break;
		}
		Tick tick;
		tick.distance = 0.F;
		for ( Visual *part: found )
		{
			tick.parts.emplace_back( *part );
			if ( auto *mark = dynamic_cast<Line *>( part ) )
			{
				tick.distance = std::sqrt( ( ( mark->start + mark->end ) / 2.F - cross ).lengthSquared() );
			}
		}
		ticks.push_back( std::move( tick ) );
	}
	// From the crossing outward
	std::stable_sort( ticks.begin(), ticks.end(), []( const Tick &a, const Tick &b ) { return a.distance < b.distance; } );

	// The grid is the layer under everything
	Fader grid( **axes.begin() );

	const float lines_end = std::clamp( options.lines, 0.05F, 0.9F );
	const float ticks_end = std::clamp( lines_end + options.ticks, lines_end + 0.05F, 0.98F );
	// Each tick takes a share of the ticks' time to come in, and starts a little after the one before
	const float window = ticks.empty() ? 0.F : std::min( ( ticks_end - lines_end ) * 0.4F, 0.15F );

	co_await tween( stage, duration, []( float t ) { return t; },
		[&]( float t )
		{
			// 1. The lines grow out from where they cross; the arrow tips come in as they get to the ends
			const float grown = within( t, 0.F, lines_end );
			for ( const Growing &g: lines )
			{
				// (exactly as it was at the end, not a rounding off)
				g.line->start = grown >= 1.F ? g.start : lerp( g.cross, g.start, grown );
				g.line->end = grown >= 1.F ? g.end : lerp( g.cross, g.end, grown );
			}
			for ( const Fader &arrow: arrows )
			{
				arrow.set( within( grown, 0.85F, 1.F ) );
			}

			// 2. The ticks in turn, and the grid across them
			for ( size_t i = 0; i < ticks.size(); ++i )
			{
				const float start = lines_end + ( ticks_end - lines_end - window ) * ( ticks.size() > 1 ? static_cast<float>( i ) / static_cast<float>( ticks.size() - 1 ) : 0.F );
				const float amount = within( t, start, start + window );
				for ( const Fader &part: ticks[i].parts )
				{
					part.set( amount );
				}
			}
			grid.set( within( t, lines_end, ticks_end ) );

			// 3. The names
			for ( const Fader &name: names )
			{
				name.set( within( t, ticks_end, 1.F ) );
			}
		} );
}
