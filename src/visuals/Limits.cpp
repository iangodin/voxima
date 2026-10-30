#include "visuals/Limits.h"
#include "visuals/Tags.h"
#include "visuals/Layout.h"
#include "visuals/MathMetrics.h"
#include <algorithm>

Limits::Limits(
	const MathFont &font,
	float s,
	std::unique_ptr<Visual> &&nucleus,
	std::unique_ptr<Visual> &&upper,
	std::unique_ptr<Visual> &&lower )
{
	const float gap_above = font.upper_limit_gap_min() * s;
	const float rise_above = font.upper_limit_baseline_rise_min() * s;
	const float gap_below = font.lower_limit_gap_min() * s;
	const float drop_below = font.lower_limit_baseline_drop_min() * s;

	// The nucleus's baseline goes to y = 0 (a large operator's baseline anchor already
	// accounts for its shift onto the math axis)
	nucleus->position = nucleus->position.movedBy( 0.F, -( nucleus->position.y() + nucleus->baseline ) );
	const AABB n = layout_extent( *nucleus );
	const float italic = italic_correction( *nucleus );

	const ScriptBox up = upper ? measure_box( *upper ) : ScriptBox{};
	const ScriptBox low = lower ? measure_box( *lower ) : ScriptBox{};

	const float width = std::max( { n.width(), up.box.width(), low.box.width() } );
	const float center = width / 2.F;

	nucleus->position = nucleus->position.movedBy( center - n.width() / 2.F - n.min.x(), 0.F );

	if ( upper )
	{
		// Baseline at least `rise_above` over the nucleus, and its bottom `gap_above` clear of it
		const float baseline = std::max( n.max.y() + rise_above, n.max.y() + gap_above + up.depth() );
		upper->position = upper->position.movedBy(
			center + italic / 2.F - up.box.width() / 2.F - up.box.min.x(), baseline - up.baseline );
	}
	if ( lower )
	{
		// Baseline at least `drop_below` under the nucleus, and its top `gap_below` clear of it
		const float baseline = std::min( n.min.y() - drop_below, n.min.y() - gap_below - low.height() );
		lower->position = lower->position.movedBy(
			center - italic / 2.F - low.box.width() / 2.F - low.box.min.x(), baseline - low.baseline );
	}

	tag_visual( add( std::move( nucleus ) ), "base" );
	if ( upper )
	{
		tag_visual( add( std::move( upper ) ), "upper" );
	}
	if ( lower )
	{
		tag_visual( add( std::move( lower ) ), "lower" );
	}

	math_axis = font.axis_height() * s;

	// The box is as wide as the widest part, from the origin
	AABB box = layout_extent( *this );
	box.min = { 0.F, box.min.y() };
	box.max = { width, box.max.y() };
	extent = box;
}
