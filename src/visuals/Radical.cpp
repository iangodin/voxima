
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
	float content_width = bounder.bounds.max.x() - bounder.bounds.min.x();

	// 2. Calculate target height (in canvas units)
	float rule_thickness = font.radical_rule_thickness() * glyph_scale;
	float vertical_gap = font.radical_vertical_gap() * glyph_scale;
	float y_top = y_max + vertical_gap + rule_thickness;
	float target_height = y_top - y_min;

	// 3. Create radical sign (large op expects font units)
	auto radical_sign = create_math_large_op( font, U'\u221A', target_height / glyph_scale );
	*radical_sign *= glyph_scale;
	bounder.bounds = {};
	radical_sign->accept( bounder );
	float radical_width = bounder.bounds.max.x() - bounder.bounds.min.x();

	// Add content
	auto &content_ref = add( std::move( content ) );
	content_ref.position = { radical_width, 0.F };

	// Draw procedural overline
	auto overline = std::make_unique<Rectangle>();
	_overline = &add( std::move( overline ) );
	_overline->color = { 1.F, 1.F, 1.F, 1.F };
	static_cast<Rectangle *>( _overline )->half_width = content_width / 2.F;
	static_cast<Rectangle *>( _overline )->half_height = rule_thickness / 2.F;
	float overline_center_x = radical_width + ( content_width / 2.F );
	_overline->position = { overline_center_x, y_top - rule_thickness / 2.F };

	_radical_sign = &add( std::move( radical_sign ) );
	_radical_sign->position = { 0.F, 0.F };
}

Radical &Radical::operator*=( float scale )
{
	position *= scale;
	if ( _radical_sign )
	{
		*_radical_sign *= scale;
	}
	if ( _overline )
	{
		_overline->position = { _overline->position.x() * scale, _overline->position.y() * scale };
		auto *rect = static_cast<Rectangle *>( _overline );
		rect->half_width *= scale;
		rect->half_height *= scale;
	}
	return *this;
}
