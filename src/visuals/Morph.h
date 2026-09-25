#pragma once

#include "visuals/Group.h"
#include "visuals/MorphLeaves.h"
#include "visuals/MorphMatch.h"
#include "visuals/Path.h"

#include <functional>
#include <memory>
#include <vector>

struct MorphOptions
{
	// How leaves are paired; see LeafMatch
	LeafMatch leaves = LeafMatch::AUTO;
	AlignCosts costs;

	// Spread the start of the parts over this fraction of the morph, in drawing order (so left to right
	// through a word): 0 moves everything together, 0.5 has the last part start halfway through, each
	// taking the remaining half. Clamped to [0, 0.95].
	float stagger = 0.F;

	// Shapes each part's own progress, 0 to 1 (on top of any easing that drives set()), such as a smoothstep.
	// Applied while a part is between its start and end; beyond them it keeps extrapolating linearly.
	std::function<float( float )> part_ease;
};

// A shape that changes from one visual into another. Both are flattened to cubic outlines, their
// contours are paired and cut to match (see MorphMatch.h), and set( t ) blends the control points, colors
// and stroke widths: t = 0 is `from`, t = 1 is `to`, and values outside that extrapolate (elastic and
// back easing overshoot).
//
// Leaves of the two visuals (each glyph, each shape) are paired up by MorphOptions::leaves: by default
// with the same tags first, then identical glyphs in reading order, then the closest. Paired leaves
// morph into each other (an identical pair just moves); a leaf with no partner fades out or in.
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

	// How the leaves were paired (indices into to_outline() of each visual, -1 for none)
	const std::vector<LeafPair> &pairs() const
	{
		return _pairs;
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
		float delay = 0.F; // when the part starts, as a fraction of the whole morph
	};

	std::unique_ptr<Group> _owned;
	Group *_root = nullptr;
	std::vector<Part> _parts;
	std::vector<LeafPair> _pairs;
	float _stagger = 0.F;
	std::function<float( float )> _part_ease;
	float _t = 0.F;
};
