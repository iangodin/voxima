#pragma once

#include "visuals/Outline.h"

#include <vector>

// Knobs of the contour alignment. Lengths are relative to the contour (arc fractions, or distances in
// units of the contour's half diagonal) so the same values work for any size.
struct AlignCosts
{
	// A junction turning more than this (radians) is a corner
	float corner_angle = 0.52F;
	// A smooth point whose tangent is this close to horizontal or vertical (sine of the angle) is an
	// extremum. Fonts put on-curve points there, and a circle has four.
	float axis_tolerance = 0.06F;

	// Weights of a match's mismatch in arc position, on-screen position and tangent direction
	float w_arc = 1.F;
	float w_position = 0.5F;
	float w_tangent = 0.3F;

	// Extra cost of matching two kinds of knot (see Knot kinds in MorphMatch.cpp)
	float corner_corner = 0.F;
	float extremum_extremum = 0.F;
	float smooth_smooth = 0.02F;
	float corner_extremum = 0.25F;
	float corner_smooth = 0.6F;
	float extremum_smooth = 0.2F;

	// Cost of leaving a knot unmatched (the other contour is split to make a partner for it)
	float skip_corner = 1.F;
	float skip_extremum = 0.15F;
	float skip_smooth = 0.03F;
	// ... and of leaving a projected knot unmatched (see below); they are optional by nature
	float skip_projected = 0.02F;

	// Knots are matched by kind of feature, which alone would twist a circle into a square (its extrema
	// are a quarter turn from the square's corners' angles). So each contour also gets a knot at the
	// nearest point to every real knot of the other, taking that knot's kind; the alignment then pairs
	// points that are already close. A projection closer than this (as an arc fraction) to a knot
	// already there is not added.
	float min_projection_gap = 0.01F;

	// How many knots of the second contour are tried as the partner of the first's anchor knot. Trying all
	// of them costs a full alignment each, which adds up on outlines with hundreds of knots; the nearest few
	// (by position) are where the right one is.
	size_t max_anchors = 16;
};

// Two contours cut into the same number of cubics, ready to interpolate segment by segment
struct ContourPair
{
	Contour a;
	Contour b;
};

// Cut both contours so every segment of one has a partner on the other, pairing features with
// features: corners with corners, extrema with extrema (a circle's four extrema go to a square's four
// corners). Neither shape changes: contours are only split (de Casteljau), never reshaped. Both
// contours should wind the same way; match_contours() takes care of that.
ContourPair align_contours( const Contour &a, const Contour &b, const AlignCosts &costs = {} );

// Two open contours (strokes) cut into the same number of cubics. Their ends stay their ends; each is cut
// wherever the other has a joint, by how far along it is.
ContourPair align_open( const Contour &a, const Contour &b );

// A contour that is a single point, with the given number of (empty) segments
Contour point_contour( Point p, size_t segments );

// Pair up the contours of two shapes and align each pair. Open contours (strokes) go with open contours;
// one left without goes with a closed one as a loop that goes out and back. Holes go with holes and
// outers with outers (by where they are and how big). Contours left over on the bigger side pair with a point on the
// other shape, so they shrink away (or grow from nothing). Winding is made consistent first, so the
// fill rule keeps holes as holes throughout the morph.
std::vector<ContourPair> match_contours( const OutlineShape &a, const OutlineShape &b, const AlignCosts &costs = {} );
