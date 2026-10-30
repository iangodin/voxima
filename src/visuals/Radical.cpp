
#include "visuals/Radical.h"
#include "visuals/Tags.h"
#include "visuals/Layout.h"
#include "visuals/CreateMathLargeOperation.h"
#include "visuals/Rectangle.h"
#include <algorithm>

Radical::Radical(
	const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&content, MathStyle style )
	: Radical( font, glyph_scale, nullptr, std::move( content ), style )
{
}

Radical::Radical(
	const MathFont &font,
	float glyph_scale,
	std::unique_ptr<Visual> &&index,
	std::unique_ptr<Visual> &&content,
	MathStyle style )
{
	// 1. Measure the radicand's layout box
	const AABB content_box = layout_extent( *content );
	float y_max = content_box.max.y();
	float y_min = content_box.min.y();
	float x_min = content_box.min.x();
	float content_width = content_box.width();

	// 2. Calculate the minimum sign height (in canvas units)
	float rule_thickness = font.radical_rule_thickness() * glyph_scale;
	float vertical_gap =
		( style.is_display() ? font.radical_display_style_vertical_gap() : font.radical_vertical_gap() ) * glyph_scale;
	float target_height = ( y_max + vertical_gap + rule_thickness ) - y_min;

	// 3. Create radical sign (large op expects font units)
	auto radical_sign = create_math_large_op( font, U'\u221A', target_height / glyph_scale );
	*radical_sign *= glyph_scale;
	// The sign occupies its advance width (its layout box), not just its painted width
	const AABB sign_box = layout_extent( *radical_sign );
	float radical_width = sign_box.max.x();

	// The sign comes in discrete sizes and is usually taller than needed. Split the
	// surplus evenly above and below the radicand (TeX Rule 11) so it looks centered.
	float sign_height = sign_box.height();
	float extra = std::max( 0.F, sign_height - target_height );
	float y_top = y_max + vertical_gap + extra / 2.F + rule_thickness;

	// With an index, the sign follows: kern, index, kern. The second kern is normally
	// negative, tucking the sign under the index. As in TeX these just add up: a wide
	// index pushes the sign right, and a very narrow one can leave the sign a little left
	// of the origin. Nothing is clamped.
	float sign_x = 0.F;
	float index_x = 0.F;
	AABB index_box;
	if ( index )
	{
		index_box = layout_extent( *index );
		index_x = font.radical_kern_before_degree() * glyph_scale;
		float kern_after = font.radical_kern_after_degree() * glyph_scale;
		sign_x = index_x + index_box.width() + kern_after;
	}

	// Align the top of the radical sign with the top of the overline
	float sign_y_max = sign_box.max.y();
	radical_sign->position = {
		radical_sign->position.x() + sign_x,
		radical_sign->position.y() + ( y_top - sign_y_max ) };

	// Add content, aligning its left edge with the right edge of the radical sign
	auto &content_ref = add( std::move( content ) );
	tag_visual( content_ref, "radicand" );
	content_ref.position = content_ref.position.movedBy( sign_x + radical_width - x_min, 0.F );

	// Draw procedural overline
	auto &overline = add( std::make_unique<Rectangle>() );
	tag_visual( overline, "overline" );
	overline.color = { 1.F, 1.F, 1.F, 1.F };
	overline.half_width = content_width / 2.F;
	overline.half_height = rule_thickness / 2.F;
	float overline_center_x = sign_x + radical_width + ( content_width / 2.F );
	overline.position = { overline_center_x, y_top - rule_thickness / 2.F };

	const AABB placed_sign = layout_extent( *radical_sign );
	tag_visual( add( std::move( radical_sign ) ), "sign" );

	if ( index )
	{
		// The bottom of the index sits a fixed fraction of the way up the sign
		float raise = font.radical_degree_bottom_raise_percent() / 100.F;
		float bottom = placed_sign.min.y() + raise * placed_sign.height();
		index->position = index->position.movedBy( index_x - index_box.min.x(), bottom - index_box.min.y() );
		tag_visual( add( std::move( index ) ), "index" );
	}

	// The radicand sits on the incoming baseline (content wasn't shifted
	// vertically), so `baseline` stays at its default of 0; only the math
	// axis needs to be recorded, for centering a radical against neighboring
	// operators/relations.
	math_axis = font.axis_height() * glyph_scale;

	// The radical's own layout box. Like a TeX box it starts at the origin (the leading
	// kern), even if the sign hangs slightly to the left of it, and ends after the radicand.
	AABB box = layout_extent( *this );
	box.min = { 0.F, box.min.y() };
	extent = box;
}
