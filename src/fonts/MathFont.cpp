#include "fonts/MathFont.h"
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ot.h>
#include <harfbuzz/hb-ft.h>
#include <memory>

namespace
{
struct HbFontDeleter
{
	void operator()( hb_font_t *font ) const
	{
		hb_font_destroy( font );
	}
};

using HbFontPtr = std::unique_ptr<hb_font_t, HbFontDeleter>;

HbFontPtr create_hb_font( const FontFace &face )
{
	const int upem = face.face->units_per_EM;
	auto *hb_font = hb_ft_font_create( face.face.get(), nullptr );
	hb_font_set_scale( hb_font, upem, upem );
	return HbFontPtr( hb_font );
}
}

MathFont::MathFont( const FontFace &face )
    : _face( face )
{
}

float MathFont::axis_height() const
{
	auto hb_font = create_hb_font( _face );
	return static_cast<float>( hb_ot_math_get_constant( hb_font.get(), HB_OT_MATH_CONSTANT_AXIS_HEIGHT ) );
}

float MathFont::italic_correction( uint32_t glyph_index ) const
{
	auto hb_font = create_hb_font( _face );
	return static_cast<float>( hb_ot_math_get_glyph_italics_correction( hb_font.get(), glyph_index ) );
}

float MathFont::top_accent_attachment( uint32_t glyph_index ) const
{
	auto hb_font = create_hb_font( _face );
	return static_cast<float>( hb_ot_math_get_glyph_top_accent_attachment( hb_font.get(), glyph_index ) );
}

std::vector<MathVariant> MathFont::get_vertical_variants( char32_t codepoint ) const
{
	auto hb_font = create_hb_font( _face );
	hb_codepoint_t base_index = 0;
	hb_font_get_glyph( hb_font.get(), codepoint, 0, &base_index );

	unsigned int count = hb_ot_math_get_glyph_variants(
	    hb_font.get(), base_index, HB_DIRECTION_TTB, 0, nullptr, nullptr );

	if ( count == 0 )
	{
		return {};
	}

	std::vector<hb_ot_math_glyph_variant_t> hb_variants( count );
	hb_ot_math_get_glyph_variants(
	    hb_font.get(), base_index, HB_DIRECTION_TTB, 0, &count, hb_variants.data() );

	std::vector<MathVariant> variants;
	variants.reserve( count );
	for ( const auto &v: hb_variants )
	{
		variants.push_back( { v.glyph, static_cast<float>( v.advance ) } );
	}
	return variants;
}
