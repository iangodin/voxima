#include "visuals/Placement.h"

#include "visuals/Layout.h"

#include <algorithm>
#include <cmath>
#include <vector>

AABB box_of( const Visual &visual )
{
	return layout_extent( visual );
}

Point point_at( const Visual &visual, Anchor anchor )
{
	const AABB box = box_of( visual );
	if ( box.empty() )
	{
		return visual.position;
	}
	const float cx = ( box.min.x() + box.max.x() ) / 2.F;
	const float cy = ( box.min.y() + box.max.y() ) / 2.F;
	switch ( anchor )
	{
		case Anchor::CENTER: return { cx, cy };
		case Anchor::LEFT: return { box.min.x(), cy };
		case Anchor::RIGHT: return { box.max.x(), cy };
		case Anchor::TOP: return { cx, box.max.y() };
		case Anchor::BOTTOM: return { cx, box.min.y() };
		case Anchor::TOP_LEFT: return { box.min.x(), box.max.y() };
		case Anchor::TOP_RIGHT: return { box.max.x(), box.max.y() };
		case Anchor::BOTTOM_LEFT: return { box.min.x(), box.min.y() };
		case Anchor::BOTTOM_RIGHT: return { box.max.x(), box.min.y() };
	}
	return { cx, cy };
}

void move_to( Visual &visual, Point where, Anchor anchor )
{
	visual.position = visual.position + ( where - point_at( visual, anchor ) );
}

void next_to( Visual &placed, const Visual &fixed, Anchor side, float gap, Align align )
{
	const AABB f = box_of( fixed );
	const AABB p = box_of( placed );
	if ( f.empty() || p.empty() )
	{
		return;
	}

	float dx = 0.F;
	float dy = 0.F;
	const bool beside = side == Anchor::LEFT || side == Anchor::RIGHT;
	if ( beside )
	{
		dx = side == Anchor::RIGHT ? f.max.x() + gap - p.min.x() : f.min.x() - gap - p.max.x();
		switch ( align )
		{
			case Align::START: dy = f.max.y() - p.max.y(); break;
			case Align::END: dy = f.min.y() - p.min.y(); break;
			case Align::BASELINE: dy = fixed.position.y() - placed.position.y(); break;
			default: dy = ( f.min.y() + f.max.y() ) / 2.F - ( p.min.y() + p.max.y() ) / 2.F; break;
		}
	}
	else if ( side == Anchor::TOP || side == Anchor::BOTTOM )
	{
		dy = side == Anchor::TOP ? f.max.y() + gap - p.min.y() : f.min.y() - gap - p.max.y();
		switch ( align )
		{
			case Align::START:
			case Align::BASELINE: dx = f.min.x() - p.min.x(); break;
			case Align::END: dx = f.max.x() - p.max.x(); break;
			default: dx = ( f.min.x() + f.max.x() ) / 2.F - ( p.min.x() + p.max.x() ) / 2.F; break;
		}
	}
	placed.position = placed.position + Point{ dx, dy };
}

namespace
{

// A child's visual and box (in the group's frame), for those that take room
struct Item
{
	Visual *visual;
	AABB box;
};

std::vector<Item> items_of( Group &group )
{
	std::vector<Item> items;
	for ( const auto &child: group )
	{
		const AABB box = box_of( *child );
		if ( !box.empty() )
		{
			items.push_back( { child.get(), box } );
		}
	}
	return items;
}

// Move the items so that the middle of what they cover is at the group's origin
void center_items( const std::vector<Item> &items, const std::vector<Point> &moves )
{
	AABB covered;
	for ( size_t i = 0; i < items.size(); ++i )
	{
		covered.add( items[i].box.moved( moves[i] ) );
	}
	if ( covered.empty() )
	{
		return;
	}
	const Point middle{ ( covered.min.x() + covered.max.x() ) / 2.F, ( covered.min.y() + covered.max.y() ) / 2.F };
	for ( size_t i = 0; i < items.size(); ++i )
	{
		items[i].visual->position = items[i].visual->position + moves[i] - middle;
	}
}

void shift_items( const std::vector<Item> &items, const std::vector<Point> &moves )
{
	for ( size_t i = 0; i < items.size(); ++i )
	{
		items[i].visual->position = items[i].visual->position + moves[i];
	}
}

// Along a row (horizontal) or column (vertical): each item's place and the cross-axis alignment
void arrange_line( Group &group, const ArrangeOptions &options, bool row )
{
	const std::vector<Item> items = items_of( group );
	if ( items.empty() )
	{
		return;
	}

	// The cross-axis edge everything lines up with: the first item's, or the extreme of all of them
	float top = std::numeric_limits<float>::lowest();
	float bottom = std::numeric_limits<float>::max();
	float left = std::numeric_limits<float>::max();
	float right = std::numeric_limits<float>::lowest();
	for ( const Item &item: items )
	{
		top = std::max( top, item.box.max.y() );
		bottom = std::min( bottom, item.box.min.y() );
		left = std::min( left, item.box.min.x() );
		right = std::max( right, item.box.max.x() );
	}

	std::vector<Point> moves( items.size() );
	float along = 0.F; // where the next one starts: the x of a row, the y (going down) of a column
	for ( size_t i = 0; i < items.size(); ++i )
	{
		const AABB &b = items[i].box;
		if ( row )
		{
			float dy = 0.F;
			switch ( options.align )
			{
				case Align::START: dy = top - b.max.y(); break;
				case Align::END: dy = bottom - b.min.y(); break;
				case Align::BASELINE: dy = items.front().visual->position.y() - items[i].visual->position.y(); break;
				default: dy = ( top + bottom ) / 2.F - ( b.min.y() + b.max.y() ) / 2.F; break;
			}
			moves[i] = { along - b.min.x(), dy };
			along += b.max.x() - b.min.x() + options.gap;
		}
		else
		{
			float dx = 0.F;
			switch ( options.align )
			{
				case Align::START:
				case Align::BASELINE: dx = left - b.min.x(); break;
				case Align::END: dx = right - b.max.x(); break;
				default: dx = ( left + right ) / 2.F - ( b.min.x() + b.max.x() ) / 2.F; break;
			}
			moves[i] = { dx, along - b.max.y() };
			along -= b.max.y() - b.min.y() + options.gap;
		}
	}

	if ( options.center )
	{
		center_items( items, moves );
	}
	else
	{
		// Keep the first where it was
		const Point first = moves.front();
		for ( Point &m: moves )
		{
			m = m - first;
		}
		shift_items( items, moves );
	}
}

}

void arrange_row( Group &group, const ArrangeOptions &options )
{
	arrange_line( group, options, true );
}

void arrange_column( Group &group, const ArrangeOptions &options )
{
	arrange_line( group, options, false );
}

void arrange_grid( Group &group, size_t columns, const GridOptions &options )
{
	const std::vector<Item> items = items_of( group );
	if ( items.empty() || columns == 0 )
	{
		return;
	}
	const size_t rows = ( items.size() + columns - 1 ) / columns;

	std::vector<float> widths( columns, 0.F );
	std::vector<float> heights( rows, 0.F );
	for ( size_t i = 0; i < items.size(); ++i )
	{
		widths[i % columns] = std::max( widths[i % columns], items[i].box.max.x() - items[i].box.min.x() );
		heights[i / columns] = std::max( heights[i / columns], items[i].box.max.y() - items[i].box.min.y() );
	}

	// The cell of each column and row: its left edge, and its top edge (going down from 0)
	std::vector<float> lefts( columns, 0.F );
	for ( size_t c = 1; c < columns; ++c )
	{
		lefts[c] = lefts[c - 1] + widths[c - 1] + options.column_gap;
	}
	std::vector<float> tops( rows, 0.F );
	for ( size_t r = 1; r < rows; ++r )
	{
		tops[r] = tops[r - 1] - heights[r - 1] - options.row_gap;
	}

	std::vector<Point> moves( items.size() );
	for ( size_t i = 0; i < items.size(); ++i )
	{
		const size_t c = i % columns;
		const size_t r = i / columns;
		const AABB &b = items[i].box;
		const float w = b.max.x() - b.min.x();
		const float h = b.max.y() - b.min.y();

		float x = lefts[c];
		if ( options.horizontal == Align::CENTER )
			x += ( widths[c] - w ) / 2.F;
		else if ( options.horizontal == Align::END )
			x += widths[c] - w;

		float y = tops[r] - h; // the bottom of the child, at the top of its cell
		if ( options.vertical == Align::CENTER )
			y = tops[r] - ( heights[r] + h ) / 2.F;
		else if ( options.vertical == Align::END )
			y = tops[r] - heights[r];

		moves[i] = { x - b.min.x(), y - b.min.y() };
	}

	if ( options.center )
	{
		center_items( items, moves );
	}
	else
	{
		const Point first = moves.front();
		for ( Point &m: moves )
		{
			m = m - first;
		}
		shift_items( items, moves );
	}
}

namespace
{

// Children's boxes, with the children that take no room left out
struct Placed
{
	Visual *visual;
	AABB box;
};

std::vector<Placed> placed_children( Group &group )
{
	std::vector<Placed> placed;
	for ( const auto &child: group )
	{
		const AABB box = box_of( *child );
		if ( !box.empty() )
		{
			placed.push_back( { child.get(), box } );
		}
	}
	return placed;
}

// Spread along one axis (0 for x, 1 for y): equal gaps between the boxes, the first and last kept
void distribute( Group &group, bool horizontal )
{
	std::vector<Placed> placed = placed_children( group );
	if ( placed.size() < 3 )
	{
		return;
	}
	auto low = [&]( const Placed &p ) { return horizontal ? p.box.min.x() : p.box.min.y(); };
	auto high = [&]( const Placed &p ) { return horizontal ? p.box.max.x() : p.box.max.y(); };

	// In order along the axis (the first and last of the group's order are the ends, whichever way it runs)
	std::stable_sort( placed.begin(), placed.end(), [&]( const Placed &a, const Placed &b ) { return low( a ) + high( a ) < low( b ) + high( b ); } );

	float sizes = 0.F;
	for ( const Placed &p: placed )
	{
		sizes += high( p ) - low( p );
	}
	const float span = high( placed.back() ) - low( placed.front() );
	const float gap = ( span - sizes ) / static_cast<float>( placed.size() - 1 );

	float at = low( placed.front() );
	for ( const Placed &p: placed )
	{
		const float move = at - low( p );
		p.visual->position = p.visual->position + ( horizontal ? Point{ move, 0.F } : Point{ 0.F, move } );
		at += high( p ) - low( p ) + gap;
	}
}

}

void distribute_row( Group &group )
{
	distribute( group, true );
}

void distribute_column( Group &group )
{
	distribute( group, false );
}

void align_edges( Group &group, Anchor edge )
{
	const std::vector<Placed> placed = placed_children( group );
	if ( placed.empty() )
	{
		return;
	}
	AABB all;
	for ( const Placed &p: placed )
	{
		all.add( p.box.min );
		all.add( p.box.max );
	}
	for ( const Placed &p: placed )
	{
		float dx = 0.F;
		float dy = 0.F;
		switch ( edge )
		{
			case Anchor::LEFT: dx = all.min.x() - p.box.min.x(); break;
			case Anchor::RIGHT: dx = all.max.x() - p.box.max.x(); break;
			case Anchor::TOP: dy = all.max.y() - p.box.max.y(); break;
			case Anchor::BOTTOM: dy = all.min.y() - p.box.min.y(); break;
			case Anchor::CENTER: dx = ( all.min.x() + all.max.x() - p.box.min.x() - p.box.max.x() ) / 2.F; break;
			default: break;
		}
		p.visual->position = p.visual->position + Point{ dx, dy };
	}
}
