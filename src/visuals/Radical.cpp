
#include "visuals/Radical.h"
#include "visuals/Bounds.h"
#include "visuals/CreateMathLargeOperation.h"
#include "visuals/Rectangle.h"
#include <algorithm>

Radical::Radical( const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&content )
{
	// 1. Measure the radicand
	BoundingBoxVisitor bounder;
	content->accept( bounder );
	float y_max = bounder.bounds.max.y();
	float y_min = bounder.bounds.min.y();
	float x_min = bounder.bounds.min.x();
	float x_max = bounder.bounds.max.x();
	float content_width = x_max - x_min;

	// 2. Calculate the minimum sign height (in canvas units)
	float rule_thickness = font.radical_rule_thickness() * glyph_scale;
	float vertical_gap = font.radical_vertical_gap() * glyph_scale;
	float target_height = ( y_max + vertical_gap + rule_thickness ) - y_min;

	// 3. Create radical sign (large op expects font units)
	auto radical_sign = create_math_large_op( font, U'\u221A', target_height / glyph_scale );
	*radical_sign *= glyph_scale;
	bounder.bounds = {};
	radical_sign->accept( bounder );
	float radical_width = bounder.bounds.max.x() - bounder.bounds.min.x();

	// The sign comes in discrete sizes and is usually taller than needed. Split the
	// surplus evenly above and below the radicand (TeX Rule 11) so it looks centered.
	float sign_height = bounder.bounds.max.y() - bounder.bounds.min.y();
	float extra = std::max( 0.F, sign_height - target_height );
	float y_top = y_max + vertical_gap + extra / 2.F + rule_thickness;

	// Align the top of the radical sign with the top of the overline
	float sign_y_max = bounder.bounds.max.y();
	radical_sign->position = { radical_sign->position.x(), radical_sign->position.y() + ( y_top - sign_y_max ) };

	// Add content, aligning its left edge with the right edge of the radical sign
	auto &content_ref = add( std::move( content ) );
	content_ref.position = { radical_width - x_min, 0.F };

	// Draw procedural overline
	auto &overline = add( std::make_unique<Rectangle>() );
	overline.color = { 1.F, 1.F, 1.F, 1.F };
	overline.half_width = content_width / 2.F;
	overline.half_height = rule_thickness / 2.F;
	float overline_center_x = radical_width + ( content_width / 2.F );
	overline.position = { overline_center_x, y_top - rule_thickness / 2.F };

	add( std::move( radical_sign ) );

	// The radicand sits on the incoming baseline (content wasn't shifted
	// vertically), so `baseline` stays at its default of 0; only the math
	// axis needs to be recorded, for centering a radical against neighboring
	// operators/relations.
	math_axis = font.axis_height() * glyph_scale;
}
