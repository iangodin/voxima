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
	    _hb_font, base_index, HB_DIRECTION_TTB, 0, nullptr, nullptr );

	if ( count == 0 )
	{
		return base_index;
	}

	std::vector<hb_ot_math_glyph_variant_t> hb_variants( count );
	hb_ot_math_get_glyph_variants(
	    _hb_font, base_index, HB_DIRECTION_TTB, 0, &count, hb_variants.data() );

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
