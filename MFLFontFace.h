#pragma once

#include "mfl/abstract_font_face.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <memory>

class MFLFontFace : public mfl::abstract_font_face
{
public:
    MFLFontFace(const std::string& path);
    ~MFLFontFace();

    FT_Face get_ft_face() const { return ft_face_; }
    hb_font_t* get_hb_font() const { return hb_font_; }

    mfl::math_constants constants() const override;
    mfl::math_glyph_info glyph_info(const size_t glyph_index) const override;
    size_t glyph_index_from_code_point(const mfl::code_point char_code, const bool use_large_variant) const override;
    std::vector<mfl::size_variant> horizontal_size_variants(const mfl::code_point char_code) const override;
    std::vector<mfl::size_variant> vertical_size_variants(const mfl::code_point char_code) const override;
    std::optional<mfl::glyph_assembly> horizontal_assembly(const mfl::code_point char_code) const override;
    std::optional<mfl::glyph_assembly> vertical_assembly(const mfl::code_point char_code) const override;
    void set_size(const mfl::points size) override;

private:
    FT_Library ft_library_;
    FT_Face ft_face_;
    hb_font_t* hb_font_;
};
