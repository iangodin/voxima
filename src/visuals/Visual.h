
#pragma once

#include "visuals/Point.h"
#include "visuals/Color.h"
#include "visuals/AABB.h"

#include <optional>

struct VisualVisitor;

struct Visual
{
	Point position = { 0.F, 0.F };
	float rotation = 0.F;
	float scale_x = 1.F;
	float scale_y = 1.F;
	Color color = { 1.F, 1.F, 1.F, 1.F };

	float stroke_thickness = 0.F;
	Color stroke_color = { 1.F, 1.F, 1.F, 1.F };

	// Math layout anchors, as y-offsets in this visual's own local coordinate
	// space (added to `position` to get the anchor in the parent's frame),
	// the same convention used by bounding-box computation.
	//
	// Both default to 0, which already matches this visual's local origin -
	// correct as-is for glyphs (whose outlines are defined relative to the
	// font baseline) and for shapes drawn centered on their own origin, like
	// Circle and Rectangle (0 is their geometric center, a reasonable stand-in
	// for both anchors on content with no natural baseline).
	float baseline = 0.F;
	float math_axis = 0.F;

	// Layout extent ("box") in the same local frame as the drawn geometry, before
	// `position`. When set it overrides the drawn (ink) bounds for layout purposes, so
	// a visual can occupy more or less room than it paints: a glyph spans its advance
	// width, a fraction includes its side padding. See layout_extent().
	std::optional<AABB> extent;

	virtual ~Visual() = default;
	virtual void accept( VisualVisitor &visitor ) const = 0;

	virtual Visual &operator*=( float scale ) = 0;

protected:
	// Scales the fields common to every visual. Each operator*= calls this and then
	// scales its own geometry.
	void scale_common( float scale )
	{
		position *= scale;
		baseline *= scale;
		math_axis *= scale;
		if ( extent )
		{
			*extent *= scale;
		}
	}
};

