#include "visuals/Fraction.h"
#include "visuals/Tags.h"
#include "visuals/Layout.h"
#include "visuals/Rectangle.h"
#include <algorithm>

namespace
{
// TeX's \nulldelimiterspace (1.2pt at 10pt): the space on each side of a fraction.
// The MATH table has no constant for it.
constexpr float NULL_DELIMITER_SPACE_EM = 0.12F;
}

Fraction::Fraction(
	const MathFont &font,
	float glyph_scale,
	std::unique_ptr<Visual> &&numerator,
	std::unique_ptr<Visual> &&denominator,
	FractionOptions options )
{
	const float em = glyph_scale * static_cast<float>( font.face().face->units_per_EM );
	const float side = options.side_space ? NULL_DELIMITER_SPACE_EM * em : 0.F;

	const float axis = font.axis_height() * glyph_scale;
	const float rule = font.fraction_rule_thickness() * glyph_scale;
	const bool display = options.style.is_display();
	const float shift_up = ( display ? font.fraction_numerator_display_style_shift_up() : font.fraction_numerator_shift_up() ) * glyph_scale;
	const float shift_down = ( display ? font.fraction_denominator_display_style_shift_down() : font.fraction_denominator_shift_down() ) * glyph_scale;
	const float num_gap_min = ( display ? font.fraction_num_display_style_gap_min() : font.fraction_numerator_gap_min() ) * glyph_scale;
	const float den_gap_min = ( display ? font.fraction_denom_display_style_gap_min() : font.fraction_denominator_gap_min() ) * glyph_scale;

	const AABB num_box = layout_extent( *numerator );
	const AABB den_box = layout_extent( *denominator );

	// Extents above and below each child's own baseline
	const float num_baseline = numerator->position.y() + numerator->baseline;
	const float den_baseline = denominator->position.y() + denominator->baseline;
	const float num_depth = num_baseline - num_box.min.y();
	const float den_height = den_box.max.y() - den_baseline;

	float num_target = 0.F; // numerator baseline above the fraction's baseline
	float den_drop = 0.F;   // denominator baseline below it
	if ( options.rule )
	{
		// Numerator baseline: at least shift_up above the baseline, and clear of the rule
		num_target = std::max( shift_up, axis + rule / 2.F + num_gap_min + num_depth );
		// Denominator baseline: at least shift_down below the baseline, and clear of the rule
		den_drop = std::max( shift_down, den_height - axis + rule / 2.F + den_gap_min );
	}
	else
	{
		// No rule (TeX rule 15c): the stack's own shifts, pushed apart equally if too close
		num_target = ( display ? font.stack_top_display_style_shift_up() : font.stack_top_shift_up() ) * glyph_scale;
		den_drop = ( display ? font.stack_bottom_display_style_shift_down() : font.stack_bottom_shift_down() ) * glyph_scale;
		const float gap_min = ( display ? font.stack_display_style_gap_min() : font.stack_gap_min() ) * glyph_scale;
		const float gap = ( num_target - num_depth ) - ( den_height - den_drop );
		if ( gap < gap_min )
		{
			const float extra = ( gap_min - gap ) / 2.F;
			num_target += extra;
			den_drop += extra;
		}
	}

	// Center both children over the rule, inside the side spacing
	const float width = std::max( num_box.width(), den_box.width() );

	numerator->position = numerator->position.movedBy(
		side + ( width - num_box.width() ) / 2.F - num_box.min.x(), num_target - num_baseline );
	denominator->position = denominator->position.movedBy(
		side + ( width - den_box.width() ) / 2.F - den_box.min.x(), -den_drop - den_baseline );

	tag_visual( add( std::move( numerator ) ), "numerator" );
	tag_visual( add( std::move( denominator ) ), "denominator" );

	if ( options.rule )
	{
		auto &bar = add( std::make_unique<Rectangle>() );
		tag_visual( bar, "bar" );
		bar.half_width = width / 2.F;
		bar.half_height = rule / 2.F;
		bar.position = { side + width / 2.F, axis };
	}

	math_axis = axis;

	// The fraction's layout box includes the side spacing, which paints nothing
	AABB box = layout_extent( *this );
	box.add( { 0.F, box.min.y() } );
	box.add( { width + 2.F * side, box.max.y() } );
	extent = box;
}
