#include "visuals/Scripts.h"
#include "visuals/Layout.h"
#include "visuals/MathAssembly.h"
#include "visuals/MathGlyph.h"
#include <algorithm>

namespace
{
// A plain run of characters. TeX only applies the baseline-drop rule to nuclei that
// are boxes (a fraction, a radical, a large operator), not to characters.
bool is_glyph_run( const Visual &visual )
{
	if ( dynamic_cast<const MathGlyph *>( &visual ) )
	{
		return false;
	}
	if ( dynamic_cast<const Glyph *>( &visual ) )
	{
		return true;
	}
	if ( const auto *group = dynamic_cast<const Group *>( &visual ) )
	{
		// Composites (fractions, radicals, scripts...) record their own layout box;
		// plain text groups don't
		if ( group->extent || dynamic_cast<const MathAssembly *>( group ) || group->size() == 0 )
		{
			return false;
		}
		return std::all_of(
			group->begin(), group->end(), []( const auto &child ) { return is_glyph_run( *child ); } );
	}
	return false;
}

// How far the superscript clears the nucleus: the italic correction of its last glyph.
// Composites (fractions, radicals, ...) have none.
float italic_correction( const Visual &visual )
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

// A script's extents above and below its own baseline
struct ScriptBox
{
	AABB box;
	float baseline = 0.F;
	float height() const { return box.max.y() - baseline; }
	float depth() const { return baseline - box.min.y(); }
};

ScriptBox measure( const Visual &visual )
{
	return { layout_extent( visual ), visual.position.y() + visual.baseline };
}
}

Scripts::Scripts(
	const MathFont &font,
	float s,
	std::unique_ptr<Visual> &&nucleus,
	std::unique_ptr<Visual> &&superscript,
	std::unique_ptr<Visual> &&subscript )
{
	const float sup_shift = font.superscript_shift() * s;
	const float sub_shift = font.subscript_shift() * s;
	const float sub_top_max = font.subscript_top_max() * s;
	const float sub_drop_min = font.subscript_baseline_drop_min() * s;
	const float sup_bottom_min = font.superscript_bottom_min() * s;
	const float sup_drop_max = font.superscript_baseline_drop_max() * s;
	const float gap_min = font.sub_superscript_gap_min() * s;
	const float sup_bottom_max_with_sub = font.superscript_bottom_max_with_subscript() * s;
	const float space_after = font.space_after_script() * s;

	// Put the nucleus's baseline at y = 0 and its left edge at x = 0
	const ScriptBox n = measure( *nucleus );
	nucleus->position = nucleus->position.movedBy( -n.box.min.x(), -n.baseline );
	const float n_right = n.box.width();

	// u: baseline of the superscript above the nucleus baseline; v: subscript below it
	float u = 0.F;
	float v = 0.F;
	if ( !is_glyph_run( *nucleus ) )
	{
		u = n.height() - sup_drop_max;
		v = n.depth() + sub_drop_min;
	}

	const ScriptBox sup = superscript ? measure( *superscript ) : ScriptBox{};
	const ScriptBox sub = subscript ? measure( *subscript ) : ScriptBox{};

	if ( superscript )
	{
		u = std::max( { u, sup_shift, sup.depth() + sup_bottom_min } );
	}
	if ( subscript )
	{
		v = std::max( v, sub_shift );
		if ( !superscript )
		{
			v = std::max( v, sub.height() - sub_top_max );
		}
	}
	if ( superscript && subscript )
	{
		// Keep the scripts apart, then lift both if the superscript ended up too low
		float gap = ( u - sup.depth() ) - ( sub.height() - v );
		if ( gap < gap_min )
		{
			v += gap_min - gap;
		}
		float psi = sup_bottom_max_with_sub - ( u - sup.depth() );
		if ( psi > 0.F )
		{
			u += psi;
			v -= psi;
		}
	}

	const float italic = italic_correction( *nucleus );

	add( std::move( nucleus ) );
	if ( superscript )
	{
		superscript->position = superscript->position.movedBy(
			n_right + italic - sup.box.min.x(), u - sup.baseline );
		add( std::move( superscript ) );
	}
	if ( subscript )
	{
		subscript->position = subscript->position.movedBy( n_right - sub.box.min.x(), -v - sub.baseline );
		add( std::move( subscript ) );
	}

	math_axis = font.axis_height() * s;

	// The box starts at the origin and ends after the scripts, plus the font's spacing
	AABB box = layout_extent( *this );
	box.min = { 0.F, box.min.y() };
	box.max = { box.max.x() + space_after, box.max.y() };
	extent = box;
}
