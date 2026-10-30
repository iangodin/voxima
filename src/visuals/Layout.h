#pragma once

#include "visuals/AABB.h"
#include "visuals/Bounds.h"
#include "visuals/Group.h"

// The box a visual occupies for layout purposes, in its parent's frame (so it includes
// `visual.position`). Unlike BoundingBoxVisitor, which measures what is painted, this
// honours an explicit `extent` (glyph advance widths, padding) and falls back to the
// painted bounds for visuals that don't set one. Groups without an extent are the union
// of their children.
inline AABB layout_extent( const Visual &visual )
{
	if ( visual.extent )
	{
		return visual.extent->moved( visual.position );
	}

	if ( const auto *group = dynamic_cast<const Group *>( &visual ) )
	{
		AABB box;
		for ( const auto &child: *group )
		{
			box.add( layout_extent( *child ).moved( group->position ) );
		}
		return box;
	}

	BoundingBoxVisitor ink;
	visual.accept( ink );
	return ink.bounds;
}
