#pragma once

#include "Axes.h"

#include <visuals/Group.h>

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

enum class MarkerShape
{
	CIRCLE,
	SQUARE,
	DIAMOND,
	TRIANGLE, // pointing up
	CROSS,    // ×
	PLUS      // +
};

// How the points are joined, if they are
enum class JoinStyle
{
	NONE,
	STRAIGHT,
	SMOOTH // a curve through them (it can overshoot between points that are far apart)
};

struct ScatterOptions
{
	MarkerShape marker = MarkerShape::CIRCLE;

	// Half the marker's width, in stage units
	float size = 0.12F;

	// The color of the markers and the line; by default the next of the axes' series colors, as a graph's is
	std::optional<Color> color;

	JoinStyle line = JoinStyle::NONE;
	float thickness = 0.04F;

	// What the series is called, for a legend
	std::string name;
};

// Points on a pair of axes (scatter()): a marker for each, and a line through them if asked for. A Group: the line first (if
// there is one), then the markers in a Group of their own, one for each point in the order given, so they can be shown one after
// another, or each moved or colored. A point outside the plot has a marker that draws nothing (so numbering by point still holds),
// and it breaks the line.
class Scatter : public Group
{
public:
	Scatter( const ScatterOptions &options, Color color );

	const ScatterOptions &options() const
	{
		return _options;
	}

	// The color the series is drawn in (the one asked for, or the axes' next series color)
	Color color() const
	{
		return _color;
	}

	size_t count() const;

	// The marker for point i
	Visual &marker( size_t index ) const;

	// The line, if there is one
	Visual *line() const
	{
		return _line;
	}

private:
	friend std::unique_ptr<Scatter> scatter( const Axes &axes, const std::vector<std::pair<double, double>> &points, const ScatterOptions &options );

	ScatterOptions _options;
	Color _color;
	Visual *_line = nullptr;
	Group *_markers = nullptr;
};

// The points on the axes. Add the result with add_plot, so it moves with them and is drawn under their numbers:
//
//     auto &chart = axes.add_plot( scatter( axes, { { 1, 2.5 }, { 2, 3.1 }, { 3, 2.8 } }, { .marker = MarkerShape::DIAMOND, .line = JoinStyle::STRAIGHT } ) );
std::unique_ptr<Scatter> scatter( const Axes &axes, const std::vector<std::pair<double, double>> &points, const ScatterOptions &options = {} );
