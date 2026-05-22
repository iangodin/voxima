#include "visuals/CreateMathLargeOperation.h"
#include "visuals/CreateText.h"
#include "visuals/MathAssembly.h"
#include "visuals/MathGlyph.h"
#include "fonts/MathFont.h"
#include <harfbuzz/hb.h>
#include <harfbuzz/hb-ft.h>
#include <cmath>

namespace
{
std::unique_ptr<MathAssembly> create_assembly( const MathFont &font, char32_t op, float target_height )
{
	auto recipe = font.get_vertical_assembly( op );
	if ( recipe.parts.empty() )
	{
		return nullptr;
	}

	// 1. Calculate base height (non-extenders) and count extenders
	float base_height = 0;
	int extender_count = 0;
	for ( const auto &part: recipe.parts )
	{
		if ( part.is_extender )
		{
			extender_count++;
		}
		else
		{
			base_height += part.full_advance;
		}
	}

	if ( extender_count == 0 )
	{
		return nullptr;
	}

	// 2. Determine how many times to repeat extenders
	float min_overlap = font.min_connector_overlap();
	float extender_height = 0;
	for ( const auto &part: recipe.parts )
	{
		if ( part.is_extender ) extender_height += part.full_advance;
	}

	// Calculate num_repeats such that total_height >= target_height with min_overlap
	// target_height <= base_height + N*ext_height - (num_parts - 1)*min_overlap
	// total_parts = (recipe_size - extender_count) + N*extender_count
	// target_height <= base_height + N*ext_height - (recipe_size - ext_count + N*ext_count - 1)*min_overlap
	// target_height <= base_height - (recipe_size - ext_count - 1)*min_overlap + N*(ext_height - ext_count*min_overlap)
	float A = base_height - (static_cast<float>(recipe.parts.size()) - extender_count - 1.0f) * min_overlap;
	float B = extender_height - static_cast<float>(extender_count) * min_overlap;

	int num_repeats = 1;
	if ( B > 0 )
	{
		num_repeats = static_cast<int>( std::ceil( ( target_height - A ) / B ) );
	}
	if ( num_repeats < 1 ) num_repeats = 1;
	if ( num_repeats > 1000 ) num_repeats = 1000;

	// 3. Calculate actual overlap to hit target_height exactly
	int total_parts = ( (int)recipe.parts.size() - extender_count ) + ( num_repeats * extender_count );
	float total_unoverlapped_height = base_height + ( static_cast<float>(num_repeats) * extender_height );
	float overlap = min_overlap;
	if ( total_parts > 1 )
	{
		overlap = ( total_unoverlapped_height - target_height ) / static_cast<float>( total_parts - 1 );
	}
// 4. Construct the assembly (+Y up)
auto assembly = std::make_unique<MathAssembly>();
assembly->italicCorrection = recipe.italic_correction;
float current_y = 0;

// Parts are listed from Bottom to Top in the HarfBuzz/MATH table.
for ( const auto &part: recipe.parts )
{
	int repeats = part.is_extender ? num_repeats : 1;
	for ( int r = 0; r < repeats; ++r )
	{
		auto glyph = create_glyph( part.glyph_index, &font.face() );
		auto math_glyph = std::make_unique<MathGlyph>();
		math_glyph->font_face = glyph->font_face;
		math_glyph->index = glyph->index;
		math_glyph->commands = std::move( glyph->commands );
		math_glyph->bearingX = glyph->bearingX;
		math_glyph->bearingY = glyph->bearingY;
		math_glyph->advanceX = glyph->advanceX;
		math_glyph->advanceY = glyph->advanceY;

		// Position this part. 
		// In +Y up, the first part (bottom) is at y=current_y, 
		// then we move UP for the next part.
		math_glyph->position.y = current_y;

		assembly->add( std::move( math_glyph ) );
		current_y += ( part.full_advance - overlap );
	}
}
// Center on math axis
float assembly_height = current_y + overlap;
float midpoint = assembly_height / 2.0f;
// The assembly spans [0, assembly_height]. 
// To center it, move it down so the midpoint is at 0, then up to the math axis.
for ( auto &visual: *assembly )
{
	visual->position.y += ( font.axis_height() - midpoint );
}
	return assembly;
}
}

std::unique_ptr<Visual> create_math_large_op( const MathFont &font, char32_t op, float target_height )
{
	uint32_t selected_glyph_index = font.get_large_variant( op, target_height );

	float h = 0, d = 0;
	font.get_glyph_metrics( selected_glyph_index, h, d );

	// If largest variant is still smaller than target, try assembly
	if ( ( h + d ) < target_height )
	{
		auto assembly = create_assembly( font, op, target_height );
		if ( assembly )
		{
			return assembly;
		}
	}

	// Return variant
	auto base_glyph = create_glyph( selected_glyph_index, &font.face() );
	auto math_glyph = std::make_unique<MathGlyph>();

	math_glyph->font_face = base_glyph->font_face;
	math_glyph->index = base_glyph->index;
	math_glyph->commands = std::move( base_glyph->commands );
	math_glyph->bearingX = base_glyph->bearingX;
	math_glyph->bearingY = base_glyph->bearingY;
	math_glyph->advanceX = base_glyph->advanceX;
	math_glyph->advanceY = base_glyph->advanceY;

	math_glyph->italicCorrection = font.italic_correction( selected_glyph_index );
	math_glyph->topAccentAttachment = font.top_accent_attachment( selected_glyph_index );

	float height = 0.0f;
	float depth = 0.0f;
	font.get_glyph_metrics( selected_glyph_index, height, depth );

	// Precise Vertical Centering on the Math Axis
	// We want the midpoint of the bounding box [height, -depth] to be at axis_height.
	// The midpoint relative to the glyph origin is (height - depth) / 2.
	float midpoint = ( height - depth ) / 2.0f;
	math_glyph->position.y = font.axis_height() - midpoint;

	return math_glyph;
}
