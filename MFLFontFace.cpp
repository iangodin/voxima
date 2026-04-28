
#include "MFLFontFace.h"

#include <hb-ft.h>
#include <hb-ot.h>

#include <ranges>
#include <vector>
#include <algorithm>
#include <stdexcept>

namespace
{
    // MFL unit_distance is 65536 (1.0 pt)
    constexpr double mfl_unit_distance = 65536.0;

    std::int32_t units_to_mfl_dist(const int native_units, const int upem)
    {
		(void)upem;
		// scaled 26.6 pixels → MFL units
		// divide by (internal_font_size * 64) to normalize to em, then multiply by mfl_unit_distance
		constexpr double internal_font_size = 100.0;
		return static_cast<std::int32_t>((static_cast<double>(native_units) / (internal_font_size * 64.0)) * mfl_unit_distance);
    }

    std::vector<mfl::size_variant> get_size_variants(hb_font_t* font, const size_t glyph_index, const hb_direction_t dir, const int upem)
    {
        constexpr auto max_number_of_variants = 20;
        auto variants = std::array<hb_ot_math_glyph_variant_t, max_number_of_variants>{};
        std::uint32_t num_variants = max_number_of_variants;
        const auto glyph_codepoint = static_cast<hb_codepoint_t>(glyph_index);
        hb_ot_math_get_glyph_variants(font, glyph_codepoint, dir, 0, &num_variants, variants.data());

        if (num_variants == 0) return {};

        auto result = std::vector<mfl::size_variant>(num_variants);
        for (size_t i = 0; i < num_variants; ++i) {
            hb_glyph_extents_t extents;
            hb_font_get_glyph_extents(font, variants[i].glyph, &extents);
            const auto size = (dir == HB_DIRECTION_LTR) ? extents.width : extents.height;
            result[i] = mfl::size_variant{.glyph_index = variants[i].glyph, .size = units_to_mfl_dist(std::abs(size), upem)};
        }
        return result;
    }
}

MFLFontFace::MFLFontFace(const std::string& path)
{
    if (FT_Init_FreeType(&ft_library_) != 0) throw std::runtime_error("Could not init FreeType");
    if (FT_New_Face(ft_library_, path.c_str(), 0, &ft_face_) != 0) {
        throw std::runtime_error("Could not load font: " + path);
    }
    hb_font_ = hb_ft_font_create(ft_face_, NULL);
}

MFLFontFace::~MFLFontFace()
{
    hb_font_destroy(hb_font_);
    FT_Done_Face(ft_face_);
    FT_Done_FreeType(ft_library_);
}

mfl::math_constants MFLFontFace::constants() const
{
    int upem = ft_face_->units_per_EM;
    const auto get_constant = [&](const hb_ot_math_constant_t c) {
        return units_to_mfl_dist(hb_ot_math_get_constant(hb_font_, c), upem);
    };

    return {
        .axis_height = get_constant(HB_OT_MATH_CONSTANT_AXIS_HEIGHT),
        .fraction_rule_thickness = get_constant(HB_OT_MATH_CONSTANT_FRACTION_RULE_THICKNESS),
        .subscript_drop = get_constant(HB_OT_MATH_CONSTANT_SUBSCRIPT_BASELINE_DROP_MIN),
        .subscript_shift_down = get_constant(HB_OT_MATH_CONSTANT_SUBSCRIPT_SHIFT_DOWN),
        .superscript_drop = get_constant(HB_OT_MATH_CONSTANT_SUPERSCRIPT_BASELINE_DROP_MAX),
        .superscript_shift_up = get_constant(HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP),
        .superscript_shift_up_cramped = get_constant(HB_OT_MATH_CONSTANT_SUPERSCRIPT_SHIFT_UP_CRAMPED),
        .minimum_dual_script_gap = get_constant(HB_OT_MATH_CONSTANT_SUB_SUPERSCRIPT_GAP_MIN),
        .maximum_superscript_bottom_in_dual_script = get_constant(HB_OT_MATH_CONSTANT_SUPERSCRIPT_BOTTOM_MAX_WITH_SUBSCRIPT),
        .space_after_script = get_constant(HB_OT_MATH_CONSTANT_SPACE_AFTER_SCRIPT),
        .radical_vertical_gap = get_constant(HB_OT_MATH_CONSTANT_RADICAL_VERTICAL_GAP),
        .radical_rule_thickness = get_constant(HB_OT_MATH_CONSTANT_RADICAL_RULE_THICKNESS),
        .radical_extra_ascender = get_constant(HB_OT_MATH_CONSTANT_RADICAL_EXTRA_ASCENDER),
        .radical_kern_before_degree = get_constant(HB_OT_MATH_CONSTANT_RADICAL_KERN_BEFORE_DEGREE),
        .radical_kern_after_degree = get_constant(HB_OT_MATH_CONSTANT_RADICAL_KERN_AFTER_DEGREE),
        .radical_degree_bottom_raise_percent = 0,
        .overline_gap = get_constant(HB_OT_MATH_CONSTANT_OVERBAR_VERTICAL_GAP),
        .overline_padding = get_constant(HB_OT_MATH_CONSTANT_OVERBAR_EXTRA_ASCENDER),
        .overline_thickness = get_constant(HB_OT_MATH_CONSTANT_OVERBAR_RULE_THICKNESS),
        .underline_gap = get_constant(HB_OT_MATH_CONSTANT_UNDERBAR_VERTICAL_GAP),
        .underline_padding = get_constant(HB_OT_MATH_CONSTANT_UNDERBAR_EXTRA_DESCENDER),
        .underline_thickness = get_constant(HB_OT_MATH_CONSTANT_UNDERBAR_RULE_THICKNESS),
        .lower_limit_min_gap = get_constant(HB_OT_MATH_CONSTANT_LOWER_LIMIT_GAP_MIN),
        .upper_limit_min_gap = get_constant(HB_OT_MATH_CONSTANT_UPPER_LIMIT_GAP_MIN),
    };
}

mfl::math_glyph_info MFLFontFace::glyph_info(const size_t glyph_index) const
{
    if ( FT_Load_Glyph(ft_face_, static_cast<FT_UInt>( glyph_index ), FT_LOAD_DEFAULT) != 0 )
	{
		std::cout << "DEBUG: " << glyph_index << " error - using 0 metrics\n";
		return { glyph_index, 0, 0, 0, 0, 0 };
	}
    const auto& metrics = ft_face_->glyph->metrics;
    int upem = ft_face_->units_per_EM;

    const auto italic_correction = hb_ot_math_get_glyph_italics_correction(hb_font_, static_cast<hb_codepoint_t>(glyph_index));

	std::cout << "DEBUG: " << glyph_index << " - "
        << units_to_mfl_dist(static_cast<int>(metrics.horiAdvance), upem) << ' '
        << units_to_mfl_dist(static_cast<int>(metrics.horiBearingY), upem) << ' '
        << units_to_mfl_dist(static_cast<int>(metrics.height - metrics.horiBearingY), upem) << ' '
        << units_to_mfl_dist(static_cast<int>(italic_correction), upem) << ' '
        << units_to_mfl_dist(hb_ot_math_get_glyph_top_accent_attachment(hb_font_, static_cast<hb_codepoint_t>(glyph_index)), upem) << '\n';
    return {
        glyph_index,
        units_to_mfl_dist(static_cast<int>(metrics.horiAdvance) / 64, upem),
        units_to_mfl_dist(static_cast<int>(metrics.horiBearingY) / 64, upem),
        units_to_mfl_dist(static_cast<int>(metrics.height - metrics.horiBearingY) / 64, upem),
        units_to_mfl_dist(static_cast<int>(italic_correction), upem),
        units_to_mfl_dist(hb_ot_math_get_glyph_top_accent_attachment(hb_font_, static_cast<hb_codepoint_t>(glyph_index)), upem)
    };
}

size_t MFLFontFace::glyph_index_from_code_point(const mfl::code_point char_code, const bool use_large_variant) const
{
    hb_codepoint_t base_index = 0;
    if (!hb_font_get_glyph(hb_font_, char_code, 0, &base_index)) {
        return 0; // .notdef
    }

    if (!use_large_variant) {
        return base_index;
    }

    // Ask HarfBuzz for the vertical size variants (most large ops grow vertically)
    constexpr uint32_t max_variants = 20;
    std::array<hb_ot_math_glyph_variant_t, max_variants> variants{};
    uint32_t count = max_variants;
    hb_ot_math_get_glyph_variants(hb_font_, base_index, HB_DIRECTION_BTT, 0, &count, variants.data());

    if (count > 0) {
        return variants[0].glyph; // first = smallest larger variant
    }

    // Fall back to horizontal variants (e.g. for wide operators)
    count = max_variants;
    hb_ot_math_get_glyph_variants(hb_font_, base_index, HB_DIRECTION_LTR, 0, &count, variants.data());

    if (count > 0) {
        return variants[0].glyph;
    }

    return base_index; // no variants found, return base
}

std::vector<mfl::size_variant> MFLFontFace::horizontal_size_variants(const mfl::code_point char_code) const
{
    return get_size_variants(hb_font_, glyph_index_from_code_point(char_code, false), HB_DIRECTION_LTR, ft_face_->units_per_EM);
}

std::vector<mfl::size_variant> MFLFontFace::vertical_size_variants(const mfl::code_point char_code) const
{
    return get_size_variants(hb_font_, glyph_index_from_code_point(char_code, false), HB_DIRECTION_BTT, ft_face_->units_per_EM);
}

std::optional<mfl::glyph_assembly> MFLFontFace::horizontal_assembly(const mfl::code_point /*char_code*/) const { return std::nullopt; }
std::optional<mfl::glyph_assembly> MFLFontFace::vertical_assembly(const mfl::code_point /*char_code*/) const { return std::nullopt; }
void MFLFontFace::set_size(const mfl::points /*size*/)
{
	const float internal_font_size = 100.F;
	FT_Set_Char_Size(ft_face_, 0, static_cast<FT_F26Dot6>(internal_font_size * 64.F), 0, 0);
}

