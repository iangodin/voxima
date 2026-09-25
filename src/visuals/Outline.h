#pragma once

#include "visuals/AABB.h"
#include "visuals/Color.h"
#include "visuals/Point.h"
#include "visuals/Visual.h"

#include <cstddef>
#include <vector>

// A cubic Bezier segment. Its start is the end of the previous segment (or Contour::start).
struct CubicSeg
{
	Point c1;
	Point c2;
	Point to;
};

// A closed loop of cubic segments. Fills close open paths implicitly, so every contour is closed:
// the last segment ends where the contour starts.
struct Contour
{
	Point start;
	std::vector<CubicSeg> segs;
};

// One drawn leaf (a glyph, a circle, a path ...) as filled outlines. A glyph like "o" has two
// contours, the outer and the hole; the fill rule (non-zero) makes the hole from opposite windings.
struct OutlineShape
{
	std::vector<Contour> contours;
	Color fill;
	Color stroke;
	float stroke_thickness = 0.F;
};

// Every leaf below `visual` (or the visual itself) as cubic contours, in the coordinate frame of
// its parent: the visual's own position/rotation/scale and those of its groups are baked in
// (same order as the SVG writer: translate, rotate, scale) and group colors are multiplied down.
// Leaves come out in drawing order, one OutlineShape each. Lines, arrow tips and images have no
// filled outline; they are skipped, and counted in `skipped` if it is given.
std::vector<OutlineShape> to_outline( const Visual &visual, size_t *skipped = nullptr );

// Area enclosed by the contour, positive for one winding and negative for the other
double signed_area( const Contour &contour );

// Bounds of the contour's on-curve points (segment ends)
AABB on_curve_bounds( const Contour &contour );

// Same loop, walked the other way (it starts at the same point)
Contour reversed( const Contour &contour );
