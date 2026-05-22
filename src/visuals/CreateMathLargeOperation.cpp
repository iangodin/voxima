#include "visuals/CreateMathLargeOperation.h"
#include "visuals/CreateText.h"
#include "fonts/MathFont.h"
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

std::unique_ptr<MathGlyph> create_math_large_op( const MathFont &font, char32_t op, float target_height )
{
	auto variants = font.get_vertical_variants( op );

	uint32_t selected_glyph_index = 0;

	if ( variants.empty() )
	{
		// Fallback to base glyph
		std::unique_ptr<hb_font_t, void ( * )( hb_font_t * )> hb_font(
			hb_ft_font_create( font.face().face.get(), nullptr ), hb_font_destroy );
		hb_font_get_glyph( hb_font.get(), op, 0, &selected_glyph_index );
	}
	else
	{
		selected_glyph_index = variants.back().glyph_index; // Default to largest
		for ( const auto &v: variants )
		{
			if ( v.advance >= target_height )
			{
				selected_glyph_index = v.glyph_index;
				break;
			}
		}
	}

	auto base_glyph = create_glyph( selected_glyph_index, &font.face() );
	auto math_glyph = std::make_unique<MathGlyph>();

	// Move base glyph data to math glyph
	math_glyph->font_face = base_glyph->font_face;
	math_glyph->index = base_glyph->index;
	math_glyph->commands = std::move( base_glyph->commands );
	math_glyph->bearingX = base_glyph->bearingX;
	math_glyph->bearingY = base_glyph->bearingY;
	math_glyph->advanceX = base_glyph->advanceX;
	math_glyph->advanceY = base_glyph->advanceY;

	// Populate math metrics
	math_glyph->italicCorrection = font.italic_correction( selected_glyph_index );
	math_glyph->topAccentAttachment = font.top_accent_attachment( selected_glyph_index );

	// Center vertically on the math axis
	// The height of the glyph in the vertical variants is the "advance"
	float height = 0.0f;
	if ( !variants.empty() )
	{
		for ( const auto &v: variants )
		{
			if ( v.glyph_index == selected_glyph_index )
			{
				height = v.advance;
				break;
			}
		}
	}

	// If no height found from variants, we'd ideally use the bounding box.
	// For now, align its center to the axis_height.
	math_glyph->position.y = font.axis_height() - ( height / 2.0f );

	return math_glyph;
}
