#include "visuals/CreateMathLargeOperation.h"
#include "fonts/MathFont.h"
#include "visuals/CreateText.h"
#include "visuals/MathAssembly.h"
#include "visuals/MathGlyph.h"
#include <cmath>
#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb.h>
#include <iostream>
#include <iomanip>

namespace
{
std::unique_ptr<MathAssembly> create_assembly(
	const MathFont &font, char32_t op, float target_height )
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
		if ( part.is_extender )
		{
			extender_height += part.full_advance;
		}
	}

	// Calculate num_repeats such that total_height >= target_height with min_overlap
	float A = base_height -
	          ( static_cast<float>( recipe.parts.size() ) - extender_count - 1.0f ) * min_overlap;
	float B = extender_height - static_cast<float>( extender_count ) * min_overlap;

	int num_repeats = 1;
	if ( B > 0 )
	{
		num_repeats = static_cast<int>( std::ceil( ( target_height - A ) / B ) );
	}
	if ( num_repeats < 1 )
	{
		num_repeats = 1;
	}
	if ( num_repeats > 1000 )
	{
		num_repeats = 1000;
	}

	// 3. Expand the recipe into a concrete list of parts
	std::vector<MathGlyphPart> concrete_parts;
	for ( const auto &part: recipe.parts )
	{
		int repeats = part.is_extender ? num_repeats : 1;
		for ( int r = 0; r < repeats; ++r )
		{
			concrete_parts.push_back( part );
		}
	}

	// 4. Calculate per-joint overlap constraints
	struct Joint
	{
		float min;
		float max;
		float capacity;
		float actual;
	};
	std::vector<Joint> joints;
	float total_full_advance = 0;
	float total_min_overlap = 0;
	float total_capacity = 0;

	for ( size_t i = 0; i < concrete_parts.size(); ++i )
	{
		total_full_advance += concrete_parts[i].full_advance;
		if ( i < concrete_parts.size() - 1 )
		{
			float max_overlap = std::min(
				concrete_parts[i].end_connector_length, concrete_parts[i + 1].start_connector_length );
			float joint_min = std::min( min_overlap, max_overlap );

			Joint joint;
			joint.min = joint_min;
			joint.max = max_overlap;
			joint.capacity = joint.max - joint.min;
			joint.actual = joint.min;

			joints.push_back( joint );
			total_min_overlap += joint.min;
			total_capacity += joint.capacity;
		}
	}

	// 5. Distribute required "shrinkage" proportionally across joints
	float required_shrinkage = total_full_advance - target_height;
	float extra_shrinkage_needed = required_shrinkage - total_min_overlap;

	if ( extra_shrinkage_needed > 0 && total_capacity > 0 )
	{
		float ratio = extra_shrinkage_needed / total_capacity;
		if ( ratio > 1.0f )
		{
			ratio = 1.0f;
		}
		for ( size_t i = 0; i < joints.size(); ++i )
		{
			joints[i].actual = joints[i].min + ( ratio * joints[i].capacity );
		}
	}

	// 6. Construct the assembly (+Y up)
	auto assembly = std::make_unique<MathAssembly>();
	assembly->italicCorrection = recipe.italic_correction;
	float current_y = 0;

	for ( size_t i = 0; i < concrete_parts.size(); ++i )
	{
		const auto &part = concrete_parts[i];
		auto glyph = create_glyph( part.glyph_index, &font.face() );
		auto math_glyph = std::make_unique<MathGlyph>();
		math_glyph->font_face = glyph->font_face;
		math_glyph->index = glyph->index;
		math_glyph->commands = std::move( glyph->commands );
		math_glyph->bearingX = glyph->bearingX;
		math_glyph->bearingY = glyph->bearingY;
		math_glyph->advanceX = glyph->advanceX;
		math_glyph->advanceY = glyph->advanceY;

		math_glyph->position = math_glyph->position.withY( current_y + ( part.full_advance - math_glyph->bearingY ) );
		assembly->add( std::move( math_glyph ) );

		if ( i < joints.size() )
		{
			current_y += ( part.full_advance - joints[i].actual );
		}
		else
		{
			current_y += part.full_advance;
		}
	}

	// Center on math axis
	float assembly_height = current_y;
	float midpoint = assembly_height / 2.0f;
	float final_offset = font.axis_height() - midpoint;
	for ( auto &visual: *assembly )
	{
		visual->position = visual->position.movedBy( 0, final_offset );
	}

	return assembly;
}
}

std::unique_ptr<Visual> create_math_large_op(
	const MathFont &font, char32_t op, float target_height )
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
	math_glyph->position = math_glyph->position.withY( font.axis_height() - midpoint );

	return math_glyph;
}
