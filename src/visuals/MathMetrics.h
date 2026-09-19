#pragma once

#include "visuals/AABB.h"
#include "visuals/Layout.h"
#include "visuals/MathAssembly.h"
#include "visuals/MathGlyph.h"

// How far a superscript (or an upper limit of a slanted operator) clears a nucleus: the
// italic correction of its last glyph. Composites (fractions, radicals, ...) have none.
inline float italic_correction( const Visual &visual )
{
	if ( const auto *glyph = dynamic_cast<const Glyph *>( &visual ) )
	{
		return glyph->italicCorrection;
	}
	if ( const auto *assembly = dynamic_cast<const MathAssembly *>( &visual ) )
	{
		return assembly->italicCorrection;
	}
	if ( const auto *group = dynamic_cast<const Group *>( &visual ) )
	{
		if ( !group->extent && group->size() > 0 )
		{
			return italic_correction( **( group->end() - 1 ) );
		}
	}
	return 0.F;
}

// A visual's layout box together with the y of its baseline, so its height and depth
// (the extents above and below the baseline) can be read off directly.
struct ScriptBox
{
	AABB box;
	float baseline = 0.F;
	float height() const { return box.max.y() - baseline; }
	float depth() const { return baseline - box.min.y(); }
};

inline ScriptBox measure_box( const Visual &visual )
{
	return { layout_extent( visual ), visual.position.y() + visual.baseline };
}
