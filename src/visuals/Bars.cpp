#include "visuals/Bars.h"
#include "visuals/MathMetrics.h"
#include "visuals/Rectangle.h"

Overline::Overline( const MathFont &font, float s, std::unique_ptr<Visual> &&content )
{
	const float gap = font.overbar_vertical_gap() * s;
	const float thickness = font.overbar_rule_thickness() * s;
	const float extra = font.overbar_extra_ascender() * s;

	const ScriptBox b = measure_box( *content );
	content->position = content->position.movedBy( -b.box.min.x(), -b.baseline );
	const float width = b.box.width();
	const float bar_bottom = b.height() + gap;

	add( std::move( content ) );
	auto &bar = add( std::make_unique<Rectangle>() );
	bar.half_width = width / 2.F;
	bar.half_height = thickness / 2.F;
	bar.position = { width / 2.F, bar_bottom + thickness / 2.F };

	math_axis = font.axis_height() * s;

	AABB box = layout_extent( *this );
	box.min = { 0.F, box.min.y() };
	box.max = { width, bar_bottom + thickness + extra };
	extent = box;
}

Underline::Underline( const MathFont &font, float s, std::unique_ptr<Visual> &&content )
{
	const float gap = font.underbar_vertical_gap() * s;
	const float thickness = font.underbar_rule_thickness() * s;
	const float extra = font.underbar_extra_descender() * s;

	const ScriptBox b = measure_box( *content );
	content->position = content->position.movedBy( -b.box.min.x(), -b.baseline );
	const float width = b.box.width();
	const float bar_top = -b.depth() - gap;

	add( std::move( content ) );
	auto &bar = add( std::make_unique<Rectangle>() );
	bar.half_width = width / 2.F;
	bar.half_height = thickness / 2.F;
	bar.position = { width / 2.F, bar_top - thickness / 2.F };

	math_axis = font.axis_height() * s;

	AABB box = layout_extent( *this );
	box.min = { 0.F, bar_top - thickness - extra };
	box.max = { width, box.max.y() };
	extent = box;
}
