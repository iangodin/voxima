#pragma once

#include "visuals/Group.h"
#include "visuals/Image.h"
#include "visuals/MorphLeaves.h"
#include "visuals/MorphMatch.h"
#include "visuals/Path.h"

#include <functional>
#include <memory>
#include <optional>
#include <vector>

// A move that keeps a shape's form: turned by `angle` (radians) and scaled by `scale` about `pivot` (the point
// that stays where it is), or, with no turn or scale, just shifted
struct Similarity
{
	bool shift_only = true;
	Point shift;
	Point pivot;
	float angle = 0.F;
	float log_scale = 0.F;

	// Where p is when a fraction t of the way: the turn and the scale go on evenly (a spiral about the pivot)
	Point apply( Point p, float t ) const;

	// The similarity that takes the points of `a` onto those of `b` (same contours, point for point), if one
	// does to within a hair of their size
	static std::optional<Similarity> between( const std::vector<Contour> &a, const std::vector<Contour> &b );
};

struct MorphOptions
{
	// How leaves are paired; see LeafMatch
	LeafMatch leaves = LeafMatch::AUTO;

	// What pure white is drawn as (Stage::foreground): what has no color is blended as the color it is seen in. morph() and
	// morph_driven() set it from the stage.
	Color foreground = { 1.F, 1.F, 1.F, 1.F };
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
// A picture (Image) is not an outline: it can only turn into the same picture, moving to where that is, and
// otherwise cross-fades with whatever replaces it.
//
// Leaves of the two visuals (each glyph, each shape) are paired up by MorphOptions::leaves: by default
// with the same tags first, then identical glyphs in reading order, then the closest. Paired leaves
// morph into each other; a leaf with no partner fades out or in. A pair that is the same shape, only moved,
// turned or scaled (the head of an arrow that turns) is carried there whole, turning about the point that stays
// put, so leaves that turn together (the whole arrow) keep together and keep their form.
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
		// For a picture instead: it moves from one place to the other and fades in or out
		Image *image = nullptr;
		ImageInfo image_a;
		ImageInfo image_b;
		std::vector<ContourPair> contours;
		// The outlines as they are, not cut to match: drawn when the part is exactly at its start or end, so
		// it looks just like the visual it came from or becomes (a stroke stays a stroke, not a loop out and
		// back, and the rasteriser draws the same curves the same way)
		std::vector<Contour> original_a;
		std::vector<Contour> original_b;
		// The same shape on both sides, only moved, turned or scaled (an arrow head when the arrow turns): it is
		// carried there whole instead of point by point, which would cut across the turn and squash it
		std::optional<Similarity> carry;
		Color fill_a, fill_b;
		Color stroke_a, stroke_b;
		float thickness_a = 0.F;
		float thickness_b = 0.F;
		float delay = 0.F; // when the part starts, as a fraction of the whole morph
		bool bevel = false; // a stroke becoming a fill, or the other way: bevelled joins in between (see Morph())
	};

	std::unique_ptr<Group> _owned;
	Group *_root = nullptr;
	std::vector<Part> _parts;
	std::vector<LeafPair> _pairs;
	float _stagger = 0.F;
	std::function<float( float )> _part_ease;
	float _t = 0.F;
};
