#pragma once

#include "visuals/Group.h"
#include "visuals/MorphMatch.h"
#include "visuals/Path.h"

#include <memory>
#include <vector>

struct MorphOptions
{
	AlignCosts costs;
};

// A shape that changes from one visual into another. Both are flattened to cubic outlines, their
// contours are paired and cut to match (see MorphMatch.h), and set( t ) blends the control points, colors
// and stroke widths: t = 0 is `from`, t = 1 is `to`, and values outside that extrapolate (elastic and
// back easing overshoot).
//
// Leaves of the two visuals are paired in drawing order; a leaf with no partner fades out (or in).
//
// The morph draws itself as a Group of Paths in the coordinate frame of the visuals' parent (see
// to_outline()). Put visual() on the stage, or take it with release() and add it yourself; it must stay
// alive for as long as set() is called.
class Morph
{
public:
	Morph( const Visual &from, const Visual &to, const MorphOptions &options = {} );

	Group &visual()
	{
		return *_root;
	}

	// Hands over the drawn Group. The Morph keeps updating it, so keep it alive while calling set().
	std::unique_ptr<Group> release()
	{
		return std::move( _owned );
	}

	void set( float t );

	float t() const
	{
		return _t;
	}

private:
	struct Part
	{
		Path *path = nullptr;
		std::vector<ContourPair> contours;
		Color fill_a, fill_b;
		Color stroke_a, stroke_b;
		float thickness_a = 0.F;
		float thickness_b = 0.F;
	};

	std::unique_ptr<Group> _owned;
	Group *_root = nullptr;
	std::vector<Part> _parts;
	float _t = 0.F;
};
