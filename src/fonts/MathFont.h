#pragma once

#include "fonts/FontFace.h"
#include <vector>
#include <cstdint>
#include <memory>

typedef struct hb_font_t hb_font_t;

struct MathVariant
{
	uint32_t glyph_index;
	float advance;
};

struct MathGlyphPart
{
	uint32_t glyph_index;
	bool is_extender;
	float start_connector_length;
	float end_connector_length;
	float full_advance;
};

struct MathAssemblyRecipe
{
	std::vector<MathGlyphPart> parts;
	float italic_correction;
};

class MathFont
{
public:
	MathFont( const FontFace &face );
	~MathFont();

	// Disable copying because of hb_font_t ownership
	MathFont( const MathFont & ) = delete;
	MathFont &operator=( const MathFont & ) = delete;

	[[nodiscard]] float axis_height() const;
	[[nodiscard]] float italic_correction( uint32_t glyph_index ) const;
	[[nodiscard]] float top_accent_attachment( uint32_t glyph_index ) const;
	[[nodiscard]] float radical_rule_thickness() const;
	[[nodiscard]] float radical_vertical_gap() const;
	// Script placement, in font units. Shifts are magnitudes as stored in the table: a
	// superscript is raised by superscript_shift, a subscript lowered by subscript_shift.
	[[nodiscard]] float superscript_shift() const;
	[[nodiscard]] float subscript_shift() const;
	[[nodiscard]] float subscript_top_max() const;
	[[nodiscard]] float subscript_baseline_drop_min() const;
	[[nodiscard]] float superscript_bottom_min() const;
	[[nodiscard]] float superscript_baseline_drop_max() const;
	[[nodiscard]] float sub_superscript_gap_min() const;
	[[nodiscard]] float superscript_bottom_max_with_subscript() const;
	[[nodiscard]] float space_after_script() const;
	// Large operators and their limits, in font units
	[[nodiscard]] float upper_limit_gap_min() const;
	[[nodiscard]] float upper_limit_baseline_rise_min() const;
	[[nodiscard]] float lower_limit_gap_min() const;
	[[nodiscard]] float lower_limit_baseline_drop_min() const;
	[[nodiscard]] float display_operator_min_height() const;
	// A fraction without a rule (a stack), in font units
	[[nodiscard]] float stack_top_shift_up() const;
	[[nodiscard]] float stack_bottom_shift_down() const;
	[[nodiscard]] float stack_gap_min() const;
	// Accents and bars, in font units
	[[nodiscard]] float accent_base_height() const;
	[[nodiscard]] float overbar_vertical_gap() const;
	[[nodiscard]] float overbar_rule_thickness() const;
	[[nodiscard]] float overbar_extra_ascender() const;
	[[nodiscard]] float underbar_vertical_gap() const;
	[[nodiscard]] float underbar_rule_thickness() const;
	[[nodiscard]] float underbar_extra_descender() const;
	// The display-style versions of the above, and the cramped superscript shift
	[[nodiscard]] float fraction_numerator_display_style_shift_up() const;
	[[nodiscard]] float fraction_denominator_display_style_shift_down() const;
	[[nodiscard]] float fraction_num_display_style_gap_min() const;
	[[nodiscard]] float fraction_denom_display_style_gap_min() const;
	[[nodiscard]] float stack_top_display_style_shift_up() const;
	[[nodiscard]] float stack_bottom_display_style_shift_down() const;
	[[nodiscard]] float stack_display_style_gap_min() const;
	[[nodiscard]] float radical_display_style_vertical_gap() const;
	[[nodiscard]] float superscript_shift_cramped() const;
	// Percentages (e.g. 70 and 50), not distances
	[[nodiscard]] float script_percent_scale_down() const;
	[[nodiscard]] float script_script_percent_scale_down() const;
	[[nodiscard]] float radical_kern_before_degree() const;
	[[nodiscard]] float radical_kern_after_degree() const;
	// Percentage (e.g. 60), not a distance
	[[nodiscard]] float radical_degree_bottom_raise_percent() const;
	[[nodiscard]] float fraction_rule_thickness() const;
	[[nodiscard]] float fraction_numerator_shift_up() const;
	[[nodiscard]] float fraction_numerator_gap_min() const;
	[[nodiscard]] float fraction_denominator_shift_down() const;
	[[nodiscard]] float fraction_denominator_gap_min() const;

	[[nodiscard]] uint32_t get_glyph_index( char32_t codepoint ) const;
	[[nodiscard]] uint32_t get_large_variant( char32_t codepoint, float target_height ) const;
	[[nodiscard]] MathAssemblyRecipe get_vertical_assembly( char32_t codepoint ) const;
	[[nodiscard]] float min_connector_overlap() const;

	// The same, but growing left to right instead of bottom to top: a stretchy glyph such as
	// a brace (over/underbrace), a wide accent (widehat, widetilde) or a labeled arrow
	[[nodiscard]] uint32_t get_large_variant_horizontal( char32_t codepoint, float target_width ) const;
	[[nodiscard]] MathAssemblyRecipe get_horizontal_assembly( char32_t codepoint ) const;
	[[nodiscard]] float min_connector_overlap_horizontal() const;

	void get_glyph_metrics( uint32_t index, float &height, float &depth ) const;
	// The glyph's ink width, for choosing a horizontal variant large enough
	[[nodiscard]] float get_glyph_width( uint32_t index ) const;

	const FontFace &face() const { return _face; }

private:
	[[nodiscard]] uint32_t get_variant( char32_t codepoint, float target_size, bool horizontal ) const;
	[[nodiscard]] MathAssemblyRecipe get_assembly( char32_t codepoint, bool horizontal ) const;

	FontFace _face;
	hb_font_t *_hb_font = nullptr;
};
