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

	[[nodiscard]] uint32_t get_glyph_index( char32_t codepoint ) const;
	[[nodiscard]] uint32_t get_large_variant( char32_t codepoint, float target_height ) const;
	[[nodiscard]] MathAssemblyRecipe get_vertical_assembly( char32_t codepoint ) const;
	[[nodiscard]] float min_connector_overlap() const;

	void get_glyph_metrics( uint32_t index, float &height, float &depth ) const;

	const FontFace &face() const { return _face; }

private:
	FontFace _face;
	hb_font_t *_hb_font = nullptr;
};
