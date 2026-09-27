#pragma once

#include "Axes.h"

#include <core/Scene.h>

#include <chrono>

struct Stage;

struct DrawAxesOptions
{
	// How the time is shared out (fractions of it): the lines grow from where they cross (with the arrow tips fading in at the
	// end of that), then the ticks and their numbers appear one after another from the crossing outward while the grid comes up,
	// and the names come in at the end.
	float lines = 0.35F;
	float ticks = 0.5F; // the rest, 1 - lines - ticks, is the names' (and the last of the ticks' fading in)
};

// Draw a pair of axes on, from nothing to all of them: the two lines growing out from where they cross, the ticks and numbers
// appearing in turn from there outward, the grid fading in, then the axis names. What is plotted on the axes (add_plot) is not
// touched: draw it after (see draw() for a graph), so the axes are there for it. It leaves them exactly as they were.
//
//     auto &axes = stage.visuals.add( make_axes( *stage.theme, { .x = { -4, 4 }, .y = { -2, 2 } } ) );
//     co_await draw_axes( stage, axes, 2s );
//     co_await draw( stage, axes.add_plot( plot( axes, f ) ), 2s );
Scene draw_axes( const Stage &stage, Axes &axes, std::chrono::duration<double> duration, const DrawAxesOptions &options = {} );
