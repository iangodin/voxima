#include "fonts/MathFont.h"
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>
#include <harfbuzz/hb-ft.h>

MathFont::MathFont( const FontFace &face )
    : _face( face )
{
	const int upem = _face.face->units_per_EM;
	_hb_font = hb_ft_font_create( _face.face.get(), nullptr );
	hb_font_set_scale( _hb_font, upem, upem );
}

MathFont::~MathFont()
{
	if ( _hb_font )
	{
		hb_font_destroy( _hb_font );
	}
}

float MathFont::axis_height() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_AXIS_HEIGHT ) );
}

float MathFont::italic_correction( uint32_t glyph_index ) const
{
	return static_cast<float>( hb_ot_math_get_glyph_italics_correction( _hb_font, glyph_index ) );
}

float MathFont::top_accent_attachment( uint32_t glyph_index ) const
{
	return static_cast<float>( hb_ot_math_get_glyph_top_accent_attachment( _hb_font, glyph_index ) );
}

float MathFont::radical_rule_thickness() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_RADICAL_RULE_THICKNESS ) );
}

float MathFont::radical_vertical_gap() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP ) );
}

float MathFont::superscript_shift() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP ) );
}

float MathFont::subscript_shift() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN ) );
}

float MathFont::subscript_top_max() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SUBSCRIPT_TOP_MAX ) );
}

float MathFont::subscript_baseline_drop_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SUBSCRIPT_BASELINE_DROP_MIN ) );
}

float MathFont::superscript_bottom_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MIN ) );
}

float MathFont::superscript_baseline_drop_max() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BASELINE_DROP_MAX ) );
}

float MathFont::sub_superscript_gap_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SUB_SUPERSCRIPT_GAP_MIN ) );
}

float MathFont::superscript_bottom_max_with_subscript() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MAX_WITH_SUBSCRIPT ) );
}

float MathFont::space_after_script() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SPACE_AFTER_SCRIPT ) );
}

float MathFont::upper_limit_gap_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN ) );
}

float MathFont::upper_limit_baseline_rise_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_UPPER_LIMIT_BASELINE_RISE_MIN ) );
}

float MathFont::lower_limit_gap_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_LOWER_LIMIT_GAP_MIN ) );
}

float MathFont::lower_limit_baseline_drop_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_LOWER_LIMIT_BASELINE_DROP_MIN ) );
}

float MathFont::display_operator_min_height() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_DISPLAY_OPERATOR_MIN_HEIGHT ) );
}

float MathFont::script_percent_scale_down() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SCRIPT_PERCENT_SCALE_DOWN ) );
}

float MathFont::script_script_percent_scale_down() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_SCRIPT_SCRIPT_PERCENT_SCALE_DOWN ) );
}

float MathFont::radical_kern_before_degree() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_RADICAL_KERN_BEFORE_DEGREE ) );
}

float MathFont::radical_kern_after_degree() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE ) );
}

float MathFont::radical_degree_bottom_raise_percent() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT ) );
}

float MathFont::fraction_rule_thickness() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_FRACTION_RULE_THICKNESS ) );
}

float MathFont::fraction_numerator_shift_up() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_SHIFT_UP ) );
}

float MathFont::fraction_numerator_gap_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_GAP_MIN ) );
}

float MathFont::fraction_denominator_shift_down() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_SHIFT_DOWN ) );
}

float MathFont::fraction_denominator_gap_min() const
{
	return static_cast<float>( hb_ot_math_get_constant( _hb_font, HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_GAP_MIN ) );
}

uint32_t MathFont::get_glyph_index( char32_t codepoint ) const
{
	hb_codepoint_t index = 0;
	hb_font_get_glyph( _hb_font, codepoint, 0, &index );
	return index;
}

uint32_t MathFont::get_large_variant( char32_t codepoint, float target_height ) const
{
	hb_codepoint_t base_index = get_glyph_index( codepoint );

	unsigned int count = hb_ot_math_get_glyph_variants(
	    _hb_font, base_index, HB_DIRECTION_BTT, 0, nullptr, nullptr );

	if ( count == 0 )
	{
		return base_index;
	}

	std::vector<hb_ot_math_glyph_variant_t> hb_variants( count );
	hb_ot_math_get_glyph_variants(
	    _hb_font, base_index, HB_DIRECTION_BTT, 0, &count, hb_variants.data() );

	uint32_t selected = hb_variants.back().glyph; // Default to largest
	for ( const auto &v: hb_variants )
	{
		if ( static_cast<float>( v.advance ) >= target_height )
		{
			selected = v.glyph;
			break;
		}
	}
	return selected;
}

MathAssemblyRecipe MathFont::get_vertical_assembly( char32_t codepoint ) const
{
	hb_codepoint_t base_index = get_glyph_index( codepoint );

	unsigned int count = 8;
	std::vector<hb_ot_math_glyph_part_t> hb_parts( count );
	hb_position_t italics_correction = 0;

	hb_ot_math_get_glyph_assembly(
	    _hb_font, base_index, HB_DIRECTION_BTT, 0, &count, hb_parts.data(), &italics_correction );

	if ( count > hb_parts.size() )
	{
		hb_parts.resize( count );
		hb_ot_math_get_glyph_assembly(
		    _hb_font, base_index, HB_DIRECTION_BTT, 0, &count, hb_parts.data(), &italics_correction );
	}

	if ( count == 0 )
	{
		return { {}, 0.0f };
	}

	std::vector<MathGlyphPart> parts;
	parts.reserve( count );
	for ( unsigned int i = 0; i < count; ++i )
	{
		parts.push_back( { hb_parts[i].glyph,
		                   ( hb_parts[i].flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER ) != 0,
		                   static_cast<float>( hb_parts[i].start_connector_length ),
		                   static_cast<float>( hb_parts[i].end_connector_length ),
		                   static_cast<float>( hb_parts[i].full_advance ) } );
	}
	return { parts, static_cast<float>( italics_correction ) };
}

float MathFont::min_connector_overlap() const
{
	return static_cast<float>( hb_ot_math_get_min_connector_overlap( _hb_font, HB_DIRECTION_BTT ) );
}

void MathFont::get_glyph_metrics( uint32_t index, float &height, float &depth ) const
{
	FT_Face ft_face = _face.face.get();
	if ( FT_Load_Glyph( ft_face, index, FT_LOAD_NO_SCALE ) == 0 )
	{
		height = static_cast<float>( ft_face->glyph->metrics.horiBearingY );
		depth = static_cast<float>( ft_face->glyph->metrics.height - ft_face->glyph->metrics.horiBearingY );
	}
	else
	{
		height = 0.0f;
		depth = 0.0f;
	}
}
