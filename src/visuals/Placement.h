#pragma once

#include "core/Camera.h"
#include "visuals/AABB.h"
#include "visuals/Group.h"
#include "visuals/Point.h"
#include "visuals/Visual.h"

// Putting visuals where they belong relative to each other, not to the stage. Every box here is the one layout uses
// (layout_extent: what a visual occupies, which for text is its advance widths and a line's height, not just the ink), in the
// frame of the visual's parent, its own position included. So visuals placed against each other must be children of the same
// group (or of the stage). The visual's own scale and rotation are not applied to its box, as in layout_extent.
//
//     next_to( label, circle, Anchor::BOTTOM );                       // under it, a gap between
//     move_to( title, { 0.F, 3.5F }, Anchor::TOP );                   // its top edge at y = 3.5
//     arrange_row( group, { .gap = 0.5F } );                          // its children side by side, centered
//     arrange_grid( group, 3, { .column_gap = 0.4F, .row_gap = 0.2F } );

// A point of a visual's box
enum class Anchor
{
	CENTER,
	LEFT,
	RIGHT,
	TOP,
	BOTTOM,
	TOP_LEFT,
	TOP_RIGHT,
	BOTTOM_LEFT,
	BOTTOM_RIGHT
};

// How things line up across the direction they are laid out in: along a row, up and down; along a column, left and right.
enum class Align
{
	START,   // the top edges of a row, the left edges of a column
	CENTER,
	END,     // the bottom edges of a row, the right edges of a column
	BASELINE // (a row) their origins level: text made with make_text has its baseline at its origin, so this lines up the
	         // baselines of texts of any size. For a column, the same as START.
};

// The box of a visual, in its parent's frame (empty if it draws nothing and has no extent)
AABB box_of( const Visual &visual );

// A point of a visual's box (its center, the middle of a side, or a corner), in its parent's frame
Point point_at( const Visual &visual, Anchor anchor );

// Move a visual so that a point of its box is at `where` (in its parent's frame)
void move_to( Visual &visual, Point where, Anchor anchor = Anchor::CENTER );

// Move `placed` beside `fixed`, on the given side (LEFT, RIGHT, TOP or BOTTOM), with a gap between, and lined up as
// asked. The two must be in the same frame.
void next_to( Visual &placed, const Visual &fixed, Anchor side, float gap = 0.3F, Align align = Align::CENTER );

struct ArrangeOptions
{
	float gap = 0.3F;
	Align align = Align::CENTER;

	// Leave the block centered on the group's origin (as it would be with center_group), not wherever the arrangement
	// began. Turn it off to keep the first child where it is.
	bool center = true;
};

// The children of a group side by side, left to right in their order (or top to bottom, for a column), a gap between.
// A child with nothing to draw takes no room.
void arrange_row( Group &group, const ArrangeOptions &options = {} );
void arrange_column( Group &group, const ArrangeOptions &options = {} );

struct GridOptions
{
	float column_gap = 0.3F;
	float row_gap = 0.3F;

	// Within a cell, which way a child sits: the left, middle or right of its column (START, CENTER, END), and the top,
	// middle or bottom of its row
	Align horizontal = Align::START;
	Align vertical = Align::CENTER;

	bool center = true;
};

// The children of a group in rows of `columns`, in order, a column as wide as its widest child and a row as tall as its
// tallest
void arrange_grid( Group &group, size_t columns, const GridOptions &options = {} );

// The children of a group spread so the spaces between them are equal, the outermost two staying where they are (a row
// left to right, or a column bottom to top). The children are taken in the order they lie in along the row, whatever their
// order in the group, and keep their place across it. Fewer than three children are left as they are.
void distribute_row( Group &group );
void distribute_column( Group &group );

// Line all the children of a group up on one edge of their combined box: LEFT, RIGHT, TOP or BOTTOM (CENTER lines up the
// middles of a column, on x, and TOP_LEFT... are not edges: they do nothing)
void align_edges( Group &group, Anchor edge );

// The frame of the picture at zoom 1 (the stage as it is before any camera move), pulled in by `margin` on every side: the
// area to lay things out in so nothing is at the very edge. 16 units wide, its height from the picture's shape.
AABB safe_area( const Camera &camera, float margin = 0.5F );

// Move a visual the least it takes to be inside `area`, as it is drawn (its scale applied, unlike box_of; its rotation not).
// If it is bigger than the area, its top left corner is kept.
void keep_inside( Visual &visual, const AABB &area );

// As keep_inside, but a visual too big for the area is first scaled down, its aspect kept, to fit
void fit_inside( Visual &visual, const AABB &area );
