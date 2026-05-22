#include "visuals/CreateMathLargeOperation.h"
#include "visuals/CreateText.h"
#include "fonts/MathFont.h"
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>

std::unique_ptr<MathGlyph> create_math_large_op( const MathFont &font, char32_t op, float target_height )
{
	uint32_t selected_glyph_index = font.get_large_variant( op, target_height );

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

	// Precise Vertical Centering on the Math Axis
	float height = 0.0f;
	float depth = 0.0f;
	font.get_glyph_metrics( selected_glyph_index, height, depth );

	// We want the midpoint of the bounding box [height, -depth] to be at axis_height.
	// The midpoint relative to the glyph origin is (height - depth) / 2.
	float midpoint = ( height - depth ) / 2.0f;
	math_glyph->position.y = font.axis_height() - midpoint;

	return math_glyph;
}
