#include "fonts/MFLFontFace.h"
#include "utils/FreetypeError.hpp"

#include <harfbuzz/hb-ft.h>
#include <harfbuzz/hb-ot.h>

#include <algorithm>
#include <ranges>
#include <stdexcept>

namespace
{
using namespace mfl::units_literals;

constexpr int font_unit_factor = 1024;

std::int32_t font_units_to_dist( const int u )
{
	return u * font_unit_factor;
}

void ft_set_size( FT_Face face, const mfl::points size )
{
	const auto fp_size = static_cast<FT_F26Dot6>( size.value() * 64.0 );
	if ( const auto err = FT_Set_Char_Size( face, fp_size, fp_size, 0, 0 ); err != 0 )
	{
		throw std::invalid_argument( ft_error_string( err ) );
	}
}

std::optional<mfl::glyph_assembly> get_assembly(
	hb_font_t *font, const size_t glyph_index, const hb_direction_t dir )
{
	constexpr auto max_number_of_parts = 8;
	auto parts = std::array<hb_ot_math_glyph_part_t, max_number_of_parts>{};
	std::uint32_t num_parts = max_number_of_parts;
	hb_position_t italic_correction = 0;
	const auto glyph_codepoint = static_cast<hb_codepoint_t>( glyph_index );
	hb_ot_math_get_glyph_assembly( font, glyph_codepoint, dir, 0, &num_parts, parts.data(), &italic_correction );

	if ( num_parts == 0 )
	{
		return std::nullopt;
	}

	const auto to_part = [&]( const hb_ot_math_glyph_part_t &p )
	{
		return mfl::glyph_part{
			.glyph_index = p.glyph,
			.start_connector_length = font_units_to_dist( p.start_connector_length ),
			.end_connector_length = font_units_to_dist( p.end_connector_length ),
			.full_advance = font_units_to_dist( p.full_advance ),
			.is_extender = ( p.flags & HB_OT_MATH_GLYPH_PART_FLAG_EXTENDER ) != 0 };
	};

	auto assembly_parts = std::vector<mfl::glyph_part>( num_parts );
	std::ranges::copy(
		parts
			| std::views::take( num_parts )
			| std::views::transform( to_part )
			| std::views::reverse,
		assembly_parts.begin() );

	return mfl::glyph_assembly{
		.parts = assembly_parts,
		.italic_correction = font_units_to_dist( italic_correction ) };
}
}

MFLFontFace::MFLFontFace( FT_Face ft_face )
	: ft_face_( ft_face )
{
}

mfl::math_constants MFLFontFace::constants() const
{
	std::unique_ptr<hb_font_t, decltype( &hb_font_destroy )> hb_font(
		hb_ft_font_create( ft_face_, nullptr ), hb_font_destroy );

	const auto get_constant = [&]( const hb_ot_math_constant_t c )
	{
		return font_units_to_dist( hb_ot_math_get_constant( hb_font.get(), c ) );
	};

	// The standard values for non-display style fraction shifts are huge so we divide them
	// by four here to get a more typical non-display style tex aesthetic
	const auto default_fraction_params = mfl::fraction_constants{
		get_constant( HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_SHIFT_UP ),
		get_constant( HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_SHIFT_DOWN ),
		get_constant( HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_GAP_MIN ),
		get_constant( HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_GAP_MIN ) };

	const auto display_style_fraction_params = mfl::fraction_constants{
		get_constant( HB_OT_MATH_CONSTANT_FRACTION_NUMERATOR_DISPLAY_STYLE_SHIFT_UP ),
		get_constant( HB_OT_MATH_CONSTANT_FRACTION_DENOMINATOR_DISPLAY_STYLE_SHIFT_DOWN ),
		get_constant( HB_OT_MATH_CONSTANT_FRACTION_NUM_DISPLAY_STYLE_GAP_MIN ),
		get_constant( HB_OT_MATH_CONSTANT_FRACTION_DENOM_DISPLAY_STYLE_GAP_MIN ) };

	const auto default_atop_params = mfl::fraction_constants{
		.numerator_shift_up = get_constant( HB_OT_MATH_CONSTANT_STACK_TOP_SHIFT_UP ),
		.denominator_shift_down = get_constant( HB_OT_MATH_CONSTANT_STACK_BOTTOM_SHIFT_DOWN ),
		.numerator_min_gap = get_constant( HB_OT_MATH_CONSTANT_STACK_GAP_MIN ),
		.denominator_min_gap = get_constant( HB_OT_MATH_CONSTANT_STACK_GAP_MIN )
	};

	const auto display_style_atop_params = mfl::fraction_constants{
		.numerator_shift_up = get_constant( HB_OT_MATH_CONSTANT_STACK_TOP_DISPLAY_STYLE_SHIFT_UP ),
		.denominator_shift_down = get_constant( HB_OT_MATH_CONSTANT_STACK_BOTTOM_DISPLAY_STYLE_SHIFT_DOWN ),
		.numerator_min_gap = get_constant( HB_OT_MATH_CONSTANT_STACK_DISPLAY_STYLE_GAP_MIN ),
		.denominator_min_gap = get_constant( HB_OT_MATH_CONSTANT_STACK_DISPLAY_STYLE_GAP_MIN )
	};

	return {
		.axis_height = get_constant( HB_OT_MATH_CONSTANT_AXIS_HEIGHT ),
		.fraction_rule_thickness = get_constant( HB_OT_MATH_CONSTANT_FRACTION_RULE_THICKNESS ),
		.subscript_drop = get_constant( HB_OT_MATH_CONSTANT_SUBSCRIPT_BASELINE_DROP_MIN ),
		.subscript_shift_down = get_constant( HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN ),
		.superscript_drop = get_constant( HB_OT_MATH_CONSTANT_SUPERSCRIPT_BASELINE_DROP_MAX ),
		.superscript_shift_up = get_constant( HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP ),
		.superscript_shift_up_cramped = get_constant( HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP_CRAMPED ),
		.minimum_dual_script_gap = get_constant( HB_OT_MATH_CONSTANT_SUB_SUPERSCRIPT_GAP_MIN ),
		.maximum_superscript_bottom_in_dual_script = get_constant( HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MAX_WITH_SUBSCRIPT ),
		.space_after_script = get_constant( HB_OT_MATH_CONSTANT_SPACE_AFTER_SCRIPT ),
		.radical_vertical_gap = get_constant( HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP ),
		.radical_rule_thickness = get_constant( HB_OT_MATH_CONSTANT_RADICAL_RULE_THICKNESS ),
		.radical_extra_ascender = get_constant( HB_OT_MATH_CONSTANT_RADICAL_EXTRA_ASCENDER ),
		.radical_kern_before_degree = get_constant( HB_OT_MATH_CONSTANT_RADICAL_KERN_BEFORE_DEGREE ),
		.radical_kern_after_degree = get_constant( HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE ),
		.radical_degree_bottom_raise_percent = get_constant( HB_OT_MATH_CONSTANT_RADICAL_DEGREE_BOTTOM_RAISE_PERCENT ),
		.overline_gap = get_constant( HB_OT_MATH_CONSTANT_OVERBAR_VERTICAL_GAP ),
		.overline_padding = get_constant( HB_OT_MATH_CONSTANT_OVERBAR_EXTRA_ASCENDER ),
		.overline_thickness = get_constant( HB_OT_MATH_CONSTANT_OVERBAR_RULE_THICKNESS ),
		.underline_gap = get_constant( HB_OT_MATH_CONSTANT_UNDERBAR_VERTICAL_GAP ),
		.underline_padding = get_constant( HB_OT_MATH_CONSTANT_UNDERBAR_EXTRA_DESCENDER ),
		.underline_thickness = get_constant( HB_OT_MATH_CONSTANT_UNDERBAR_RULE_THICKNESS ),
		.lower_limit_min_gap = get_constant( HB_OT_MATH_CONSTANT_LOWER_LIMIT_GAP_MIN ),
		.upper_limit_min_gap = get_constant( HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN ),
		.default_fraction = default_fraction_params,
		.display_style_fraction = display_style_fraction_params,
		.default_atop = default_atop_params,
		.display_style_atop = display_style_atop_params };
}

mfl::math_glyph_info MFLFontFace::glyph_info( const size_t glyph_index ) const
{
	std::unique_ptr<hb_font_t, decltype( &hb_font_destroy )> hb_font(
		hb_ft_font_create( ft_face_, nullptr ), hb_font_destroy );
	FT_Load_Char( ft_face_, static_cast<FT_ULong>( glyph_index ), FT_LOAD_DEFAULT );
	const auto &metrics = ft_face_->glyph->metrics;

	const auto glyph_codepoint = static_cast<hb_codepoint_t>( glyph_index );
	const auto italic_correction =
		hb_ot_math_get_glyph_italics_correction( hb_font.get(), glyph_codepoint );

	// TODO - for the StixFonts, the horizontal advance seems to include the italic correction. This
	// is generally exactly what we want and expect (positioning a tall non-slanted character after
	// a slanted character should take that italic correction into account). We can clearly see that
	// the italic correction is already in the horizontal advance on tall slanted symbols like
	// integrals, but if we subtract the italic correction
	// from the width on all symbols then superscripts are drawn too close to the top right corner
	// of italic base symbols. So we only subtract the italic correction for tall slanted symbols.
	// This appears to means that when taking, say, an italic X, the italic correction is already in
	// the width and then when positioning a superscript on that X the italic correction is applied
	// again, but this prevents superscripts from overlapping with italic symbols in the nucleus.
	const auto integral_indices = std::array{ 1699, 1705, 1711, 1717, 1723, 1729 };
	const auto integral_fix =
		( std::ranges::find( integral_indices, glyph_index ) != integral_indices.end() )
			? -italic_correction
			: 0;

	hb_glyph_extents_t extents;
	hb_font_get_glyph_extents_for_origin(
		hb_font.get(),
		glyph_codepoint,
		HB_DIRECTION_LTR,
		&extents ); // TODO what does the boolean return tell us?

	const auto height = metrics.horiBearingY;
	const auto depth =
		metrics.height -
		height; // metrics height is total glyph height - our height is height from the baseline
	return {
		glyph_index,
		font_units_to_dist( static_cast<int>( metrics.horiAdvance + integral_fix ) ),
		font_units_to_dist( static_cast<int>( height ) ),
		font_units_to_dist( static_cast<int>( depth ) ),
		font_units_to_dist( italic_correction ),
		font_units_to_dist(
			hb_ot_math_get_glyph_top_accent_attachment( hb_font.get(), glyph_codepoint ) ) };
}

size_t MFLFontFace::glyph_index_from_code_point(
	const mfl::code_point char_code, const bool use_large_variant ) const
{
	// hard wired values from the stylistic set 04 in Stix2Math which contains nicer looking primes
	constexpr auto prime_index = 8242U;
	constexpr auto offset_to_better_primes = 6792U;
	if ( ( char_code >= prime_index ) && ( char_code < prime_index + 3 ) )
	{
		return char_code - offset_to_better_primes;
	}

	std::unique_ptr<hb_font_t, decltype( &hb_font_destroy )> hb_font(
		hb_ft_font_create( ft_face_, nullptr ), hb_font_destroy );
	hb_codepoint_t base_index = 0;
	hb_font_get_glyph( hb_font.get(), char_code, 0, &base_index );

	if ( use_large_variant )
	{
		// Correct approach: Use HarfBuzz's OpenType MATH parser to get the true large variant index
		hb_ot_math_glyph_variant_t variant;
		unsigned int count = 1;

		// Query the vertical math variants for this glyph index
		hb_ot_math_get_glyph_variants(
			hb_font.get(), base_index, HB_DIRECTION_TTB, 0, &count, &variant );

		if ( count > 0 )
		{
			return variant.glyph;
		}
	}

	return base_index;
}

std::vector<mfl::size_variant> MFLFontFace::horizontal_size_variants(
	const mfl::code_point char_code ) const
{
	// Create a temporary HarfBuzz font wrapper around your FreeType face
	std::unique_ptr<hb_font_t, decltype( &hb_font_destroy )> hb_font(
		hb_ft_font_create( ft_face_, nullptr ), hb_font_destroy );

	// Resolve the standard Unicode codepoint to its baseline Glyph ID
	hb_codepoint_t base_index = 0;
	hb_font_get_glyph( hb_font.get(), char_code, 0, &base_index );

	// Query HarfBuzz for the count of horizontal size variants available for this glyph
	unsigned int count = hb_ot_math_get_glyph_variants(
		hb_font.get(), base_index, HB_DIRECTION_LTR, 0, nullptr, nullptr );

	// If the font doesn't provide any horizontal sizing alternatives, return empty
	if ( count == 0 )
	{
		return {};
	}

	// Allocate space and fetch the structural variant array data from the MATH table
	std::vector<hb_ot_math_glyph_variant_t> hb_variants( count );
	hb_ot_math_get_glyph_variants(
		hb_font.get(), base_index, HB_DIRECTION_LTR, 0, &count, hb_variants.data() );

	// Translate HarfBuzz internal variant descriptions into MFL's expected format
	std::vector<mfl::size_variant> mfl_variants;
	mfl_variants.reserve( count );
	for ( const auto &v: hb_variants )
	{
		mfl_variants.push_back( {
			static_cast<size_t>( v.glyph ), // The specific Glyph ID for this size step
			font_units_to_dist( v.advance ) // The target size/advance width metric
		} );
	}

	return mfl_variants;
}

std::vector<mfl::size_variant> MFLFontFace::vertical_size_variants(
	const mfl::code_point char_code ) const
{
	std::unique_ptr<hb_font_t, decltype( &hb_font_destroy )> hb_font(
		hb_ft_font_create( ft_face_, nullptr ), hb_font_destroy );
	hb_codepoint_t base_index = 0;
	hb_font_get_glyph( hb_font.get(), char_code, 0, &base_index );

	// Find out how many variants exist
	unsigned int count = hb_ot_math_get_glyph_variants(
		hb_font.get(), base_index, HB_DIRECTION_TTB, 0, nullptr, nullptr );
	if ( count == 0 )
	{
		return {};
	}

	std::vector<hb_ot_math_glyph_variant_t> hb_variants( count );
	hb_ot_math_get_glyph_variants(
		hb_font.get(), base_index, HB_DIRECTION_TTB, 0, &count, hb_variants.data() );

	std::vector<mfl::size_variant> mfl_variants;
	mfl_variants.reserve( count );
	for ( const auto &v: hb_variants )
	{
		mfl_variants.push_back(
			{ static_cast<size_t>( v.glyph ), font_units_to_dist( v.advance ) } );
	}
	return mfl_variants;
}

std::optional<mfl::glyph_assembly> MFLFontFace::horizontal_assembly(
	const mfl::code_point char_code ) const
{
	std::unique_ptr<hb_font_t, decltype( &hb_font_destroy )> hb_font(
		hb_ft_font_create( ft_face_, nullptr ), hb_font_destroy );
	const auto glyph_index = glyph_index_from_code_point( char_code, false );
	return get_assembly( hb_font.get(), glyph_index, HB_DIRECTION_LTR );
}

std::optional<mfl::glyph_assembly> MFLFontFace::vertical_assembly(
	const mfl::code_point char_code ) const
{
	std::unique_ptr<hb_font_t, decltype( &hb_font_destroy )> hb_font(
		hb_ft_font_create( ft_face_, nullptr ), hb_font_destroy );
	const auto glyph_index = glyph_index_from_code_point( char_code, false );
	return get_assembly( hb_font.get(), glyph_index, HB_DIRECTION_BTT );
}

void MFLFontFace::set_size( const mfl::points size )
{
	ft_set_size( ft_face_, size );
}
