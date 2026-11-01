#pragma once

#include "core/Theme.h"
#include "visuals/CreateText.h"
#include "visuals/Group.h"
#include "visuals/Path.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct PolygonOptions
{
	// White is the theme's text color
	Color outline = { 1.F, 1.F, 1.F, 1.F };
	float thickness = 0.04F;

	// Filled, in this color (it is often a tint: alpha under 1); none by default
	std::optional<Color> fill = {};

	// The vertices' names ("A", "B", ...), one for each, set in math italic just outside each corner, away from the middle of
	// the shape. Fewer than the vertices is fine: the rest are left unnamed.
	std::vector<std::string> labels = {};
	FontStyle label_style = FontStyle{ Intent::MATH };
	float label_gap = 0.25F;

	// A dot on each vertex
	bool dots = false;
};

// A polygon through the points, closed: a Group of its outline (child 0), then the labels
std::unique_ptr<Group> make_polygon( Theme &theme, const std::vector<Point> &points, const PolygonOptions &options = {} );

struct AngleOptions
{
	float radius = 0.6F;
	float thickness = 0.035F;
	Color color = { 1.F, 1.F, 1.F, 1.F };

	// A right angle is marked with a little square in place of the arc
	bool square_for_right = true;

	// Its size (the name of the angle: "θ", "60°"), set just past the arc on the line that halves the angle; empty for none
	std::string label = {};
	FontStyle label_style = FontStyle{ Intent::MATH }.small();
	float label_gap = 0.25F;
};

// The angle at `vertex` between the rays toward `a` and toward `b` (the smaller of the two ways round): an arc across it,
// or a square if it is a right angle, and a label
std::unique_ptr<Group> make_angle( Theme &theme, Point vertex, Point a, Point b, const AngleOptions &options = {} );

// The angle at `vertex` between the rays toward `a` and toward `b`, in degrees, 0 to 180
float angle_between( Point vertex, Point a, Point b );
