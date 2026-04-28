#pragma once

#include "Freetype.h"

#include <mfl/abstract_font_face.hpp>
#include <mfl/font_family.hpp>
#include <mfl/units.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

class MFLFontFace : public mfl::abstract_font_face
{
public:
	MFLFontFace( const mfl::font_family family, const Freetype &ft );

	[[nodiscard]] mfl::math_constants constants() const override;
	[[nodiscard]] mfl::math_glyph_info glyph_info(const size_t glyph_index) const override;
	[[nodiscard]] size_t glyph_index_from_code_point(const mfl::code_point char_code,
													 const bool use_large_variant) const override;
	[[nodiscard]] std::vector<mfl::size_variant> horizontal_size_variants(const mfl::code_point char_code) const override;
	[[nodiscard]] std::vector<mfl::size_variant> vertical_size_variants(const mfl::code_point char_code) const override;
	[[nodiscard]] std::optional<mfl::glyph_assembly> horizontal_assembly(const mfl::code_point char_code) const override;
	[[nodiscard]] std::optional<mfl::glyph_assembly> vertical_assembly(const mfl::code_point char_code) const override;
	void set_size(const mfl::points size) override;

private:
	FT_Face ft_face_ = nullptr;
};

