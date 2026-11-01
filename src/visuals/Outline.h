#pragma once

#include "visuals/AABB.h"
#include "visuals/Color.h"
#include "visuals/Point.h"
#include "visuals/Visual.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// A cubic Bezier segment. Its start is the end of the previous segment (or Contour::start).
struct CubicSeg
{
	Point c1;
	Point c2;
	Point to;
};

// A run of cubic segments. Almost every contour is a closed loop (fills close open paths implicitly): the
// last segment ends where the contour starts. The exception is a stroke-only shape such as a Line, an open
// contour that ends where it ends.
struct Contour
{
	Point start;
	std::vector<CubicSeg> segs;
	bool closed = true;
};

// What a leaf was drawn as
enum class LeafKind
{
	PATH,
	GLYPH,
	CIRCLE,
	ELLIPSE,
	RECTANGLE,
	ROUND_RECTANGLE,
	LINE,
	ARROW_TIP,
	IMAGE
};

// Where an Image leaf ends up, in its parent's frame: what an Image needs to be drawn there
struct ImageInfo
{
	std::string path;
	float half_width = 1.F;
	float half_height = 1.F;
	Point position;
	float rotation = 0.F; // degrees
	float scale_x = 1.F;
	float scale_y = 1.F;
};

// One drawn leaf (a glyph, a circle, a path ...) as filled outlines. A glyph like "o" has two
// contours, the outer and the hole; the fill rule (non-zero) makes the hole from opposite windings.
struct OutlineShape
{
	std::vector<Contour> contours;
	Color fill;
	Color stroke;
	float stroke_thickness = 0.F;
	StrokeDash dash = {}; // scaled as the thickness is

	// Every tag on the leaf and on the groups above it (within the visual that was flattened), so
	// leaves can be matched by the names the composites give their parts
	std::vector<std::string> tags;

	// Which word of laid-out text the leaf is in (make_styled_text's "word<N>" groups), counted in drawing
	// order, or -1. Words are told apart by the group they are, not by the tag's name: every piece of text
	// (each text() in a formula) is laid out on its own and starts again at "word0".
	int word = -1;

	LeafKind kind = LeafKind::PATH;

	// Set for an Image, which has no outline (contours is empty): the picture and where it is. Its `fill`
	// carries the alpha it is drawn with.
	std::optional<ImageInfo> image;

	// Identifies the leaf's own geometry, before any transform: two glyphs of the same letter at the
	// same size have the same key wherever they sit. Never 0.
	uint64_t key = 0;
};

// Every leaf below `visual` (or the visual itself) as cubic contours, in the coordinate frame of
// its parent: the visual's own position/rotation/scale and those of its groups are baked in
// (same order as the SVG writer: translate, rotate, scale) and group colors are multiplied down.
// Leaves come out in drawing order, one OutlineShape each. Lines, arrow tips and images have no
// filled outline. (A Line, or a Path with a stroke and no fill, becomes an open contour; an ArrowTip is its triangle;
// an Image is a shape with `image` set and no contours.) Left out, and counted in `skipped` if it is given:
// an image placed with a shear, which an Image cannot be drawn as.
//
// Colors come out as they will be drawn: pure white, the color of what has none, is `foreground` (see Color::resolved), so a
// morph between shapes blends between the colors they are seen in.
std::vector<OutlineShape> to_outline( const Visual &visual, size_t *skipped = nullptr, const Color &foreground = { 1.F, 1.F, 1.F, 1.F } );

// Area enclosed by the contour, positive for one winding and negative for the other (an open contour
// encloses what it would with a straight line back to its start)
double signed_area( const Contour &contour );

// Bounds of the contour's on-curve points (segment ends)
AABB on_curve_bounds( const Contour &contour );

// Where a point given in the visual's own coordinates lands in its parent's: the visual's position, rotation
// and scale applied (in the order the SVG writer uses)
Point place_in_parent( const Visual &visual, Point p );

// Same contour, walked the other way. A loop starts at the same point; an open contour starts at its end.
Contour reversed( const Contour &contour );

// An open contour as a closed loop that goes out along it and comes straight back (it encloses nothing),
// so it can be morphed into or out of a filled shape. A closed contour is returned as it is.
Contour closed_loop( const Contour &contour );
