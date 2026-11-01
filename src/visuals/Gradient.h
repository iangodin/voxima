#pragma once

#include "visuals/AABB.h"
#include "visuals/Color.h"
#include "visuals/Point.h"

#include <vector>

struct Visual;

// A color at a place along a gradient (0 is its start, 1 its end). White is the theme's text color, as everywhere.
struct GradientStop
{
	float offset = 0.F;
	Color color = {};
};

// x' = a x + c y + tx, y' = b x + d y + ty
struct GradientTransform
{
	float a = 1.F;
	float b = 0.F;
	float c = 0.F;
	float d = 1.F;
	float tx = 0.F;
	float ty = 0.F;

	// The transform that applies `local` first, then this one
	[[nodiscard]] GradientTransform operator*( const GradientTransform &local ) const
	{
		return {
			a * local.a + c * local.b,
			b * local.a + d * local.b,
			a * local.c + c * local.d,
			b * local.c + d * local.d,
			a * local.tx + c * local.ty + tx,
			b * local.tx + d * local.ty + ty };
	}

	bool operator==( const GradientTransform & ) const = default;
};

enum class GradientKind
{
	LINEAR,
	RADIAL
};

// A fill that changes color across a shape: Visual::fill_gradient. It is placed in the visual's own coordinates (the ones its
// outline is in), so it moves, turns and scales with the visual; Visual::color tints it, and the color's alpha fades it.
//
//     box.fill_gradient = Gradient::across( box_of_it, 90.F, { { 0.F, dark }, { 1.F, bright } } ); // from the bottom up
//     disc.fill_gradient = Gradient::radial( { 0.F, 0.F }, disc.radius, { { 0.F, white }, { 1.F, blue } } );
//
// Before the first stop and after the last, the color stays what it was there. Colors are blended as they are written (in
// sRGB, not in linear light; add stops for anything else), to the 8 bits a frame has, with no dithering.
struct Gradient
{
	GradientKind kind = GradientKind::LINEAR;

	// LINEAR: the color goes from `start` to `end`, and is the same along lines across that
	Point start = { 0.F, 0.F };
	Point end = { 1.F, 0.F };

	// RADIAL: the color goes from `focus` out to the circle at `center` of `radius`
	Point center = { 0.F, 0.F };
	float radius = 1.F;
	Point focus = { 0.F, 0.F };

	std::vector<GradientStop> stops = {};

	// Places it further, as it is placed in a morph (for an ellipse, a radial one with an uneven scale)
	GradientTransform transform = {};

	static Gradient linear( Point start, Point end, std::vector<GradientStop> stops );
	static Gradient radial( Point center, float radius, std::vector<GradientStop> stops );
	static Gradient radial( Point center, float radius, Point focus, std::vector<GradientStop> stops );

	// A linear one right across a box, heading at an angle (degrees, anticlockwise: 0 goes to the right, 90 up): its start and
	// end are just far enough for the corners to be the first and last colors
	static Gradient across( const AABB &box, float degrees, std::vector<GradientStop> stops );

	// Whether it can be drawn as a gradient. If not (it goes nowhere, its radius is 0, a scale of 0 flattens it), it is
	// drawn in the last color, which is what SVG does.
	[[nodiscard]] bool drawable() const;

	// The color at a place along it (white if it has no stops)
	[[nodiscard]] Color sample( float offset ) const;

	// The color it comes to overall, as much of each as there is across it
	[[nodiscard]] Color average() const;

	// The same stops in order of offset, from 0 to 1
	[[nodiscard]] std::vector<GradientStop> ordered_stops() const;

	Gradient &operator*=( float scale );
};

// Fill every shape below `visual` (each glyph of text, say) with a gradient right across that shape, as Gradient::across does.
// Text has one per glyph, not one across the word. Lines and images have no fill, and are left as they are.
void fill_each_with_gradient( Visual &visual, float degrees, const std::vector<GradientStop> &stops );
