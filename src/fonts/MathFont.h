#pragma once

#include "fonts/FontFace.h"
#include <vector>
#include <cstdint>

struct MathVariant
{
	uint32_t glyph_index;
	float advance;
};

class MathFont
{
public:
	MathFont( const FontFace &face );

	[[nodiscard]] float axis_height() const;
	[[nodiscard]] float italic_correction( uint32_t glyph_index ) const;
	[[nodiscard]] float top_accent_attachment( uint32_t glyph_index ) const;

	[[nodiscard]] std::vector<MathVariant> get_vertical_variants( char32_t codepoint ) const;

	const FontFace &face() const { return _face; }

private:
	const FontFace &_face;
};
