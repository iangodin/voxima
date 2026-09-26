#pragma once

#include "Axes.h"

#include <core/Scene.h>
#include <visuals/Path.h>

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

struct Stage;

struct GraphOptions
{
	// Where on x the function is drawn (by default, the whole of the axes' x range)
	std::optional<Range> x;

	// How many times the function is worked out across it
	size_t samples = 400;

	// The curve's color; by default the next of the axes' series colors (the palette's blue, then orange, green ...)
	std::optional<Color> color;
	float thickness = 0.05F;
};

// The graph of a function on a pair of axes (plot()): a smooth stroked curve through samples of it, in the axes'
// coordinates. It stops at the edge of the plot area, and breaks where the function is not a number or leaves the
// plot (1/x has two pieces). It can be drawn on bit by bit (set_drawn, draw).
class FunctionGraph : public Path
{
public:
	// The pieces of the curve, each a run of points in the axes' coordinates
	const std::vector<std::vector<Point>> &pieces() const
	{
		return _pieces;
	}

	// Draw only the first `fraction` of the curve, by its length (0: nothing, 1: all of it)
	void set_drawn( float fraction );

	float drawn() const
	{
		return _drawn;
	}

	// The length of the whole curve, in the axes' coordinates
	float length() const
	{
		return _length;
	}

private:
	friend std::unique_ptr<FunctionGraph> plot( const Axes &axes, const std::function<double( double )> &f, const GraphOptions &options );

	std::vector<std::vector<Point>> _pieces;
	std::vector<std::vector<float>> _distance; // along the whole curve, at each point of each piece
	float _length = 0.F;
	float _drawn = 1.F;
};

// The graph of y = f( x ) on the axes. Add it to them with add_plot, so it moves with them and is drawn under their
// numbers:
//
//     auto &axes = stage.visuals.add( make_axes( *stage.theme, { .x = { -4, 4 }, .y = { -2, 2 } } ) );
//     auto &wave = axes.add_plot( plot( axes, []( double x ) { return std::sin( x ); } ) );
//     co_await draw( stage, wave, 2s );
std::unique_ptr<FunctionGraph> plot( const Axes &axes, const std::function<double( double )> &f, const GraphOptions &options = {} );

// Draw a graph on, from nothing to all of it, over a length of time, at an even pace along it
Scene draw( const Stage &stage, FunctionGraph &graph, std::chrono::duration<double> duration );
