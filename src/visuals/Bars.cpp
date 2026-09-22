#include "visuals/Bars.h"
#include "visuals/Tags.h"
#include "visuals/MathMetrics.h"
#include "visuals/Rectangle.h"
#include "visuals/CreateMathLargeOperation.h"

Overline::Overline( const MathFont &font, float s, std::unique_ptr<Visual> &&content )
{
	const float gap = font.overbar_vertical_gap() * s;
	const float thickness = font.overbar_rule_thickness() * s;
	const float extra = font.overbar_extra_ascender() * s;

	const ScriptBox b = measure_box( *content );
	content->position = content->position.movedBy( -b.box.min.x(), -b.baseline );
	const float width = b.box.width();
	const float bar_bottom = b.height() + gap;

	tag_visual( add( std::move( content ) ), "content" );
	auto &bar = add( std::make_unique<Rectangle>() );
	tag_visual( bar, "line" );
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

	tag_visual( add( std::move( content ) ), "content" );
	auto &bar = add( std::make_unique<Rectangle>() );
	tag_visual( bar, "line" );
	bar.half_width = width / 2.F;
	bar.half_height = thickness / 2.F;
	bar.position = { width / 2.F, bar_top - thickness / 2.F };

	math_axis = font.axis_height() * s;

	AABB box = layout_extent( *this );
	box.min = { 0.F, bar_top - thickness - extra };
	box.max = { width, box.max.y() };
	extent = box;
}

Overbrace::Overbrace( const MathFont &font, float s, std::unique_ptr<Visual> &&content )
{
	const float gap = font.overbar_vertical_gap() * s;
	const float extra = font.overbar_extra_ascender() * s;

	const ScriptBox b = measure_box( *content );
	content->position = content->position.movedBy( -b.box.min.x(), -b.baseline );
	const float width = b.box.width();

	auto brace = create_math_large_op_horizontal( font, U'⏞', width / s );
	*brace *= s;
	const AABB brace_box = layout_extent( *brace );

	// Center the brace over the content (it may come out a touch narrower or wider than asked)
	const float bottom = b.height() + gap;
	brace->position = brace->position.movedBy(
		( width - brace_box.width() ) / 2.F - brace_box.min.x(), bottom - brace_box.min.y() );

	tag_visual( add( std::move( content ) ), "content" );
	const AABB placed_brace = layout_extent( *brace );
	tag_visual( add( std::move( brace ) ), "brace" );

	math_axis = font.axis_height() * s;

	AABB box = layout_extent( *this );
	box.min = { 0.F, box.min.y() };
	box.max = { width, placed_brace.max.y() + extra };
	extent = box;
}

Underbrace::Underbrace( const MathFont &font, float s, std::unique_ptr<Visual> &&content )
{
	const float gap = font.underbar_vertical_gap() * s;
	const float extra = font.underbar_extra_descender() * s;

	const ScriptBox b = measure_box( *content );
	content->position = content->position.movedBy( -b.box.min.x(), -b.baseline );
	const float width = b.box.width();

	auto brace = create_math_large_op_horizontal( font, U'⏟', width / s );
	*brace *= s;
	const AABB brace_box = layout_extent( *brace );

	const float top = -b.depth() - gap;
	brace->position = brace->position.movedBy(
		( width - brace_box.width() ) / 2.F - brace_box.min.x(), top - brace_box.max.y() );

	tag_visual( add( std::move( content ) ), "content" );
	const AABB placed_brace = layout_extent( *brace );
	tag_visual( add( std::move( brace ) ), "brace" );

	math_axis = font.axis_height() * s;

	AABB box = layout_extent( *this );
	box.min = { 0.F, placed_brace.min.y() - extra };
	box.max = { width, box.max.y() };
	extent = box;
}
