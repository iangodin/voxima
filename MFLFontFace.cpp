#include "MFLFontFace.h"
#include <hb-ft.h>
#include <hb-ot.h>
#include <iostream>

MFLFontFace::MFLFontFace(const std::string& path)
{
    FT_Init_FreeType(&ft_library_);
    FT_New_Face(ft_library_, path.c_str(), 0, &ft_face_);
    hb_font_ = hb_ft_font_create(ft_face_, NULL);
}

MFLFontFace::~MFLFontFace()
{
    hb_font_destroy(hb_font_);
    FT_Done_Face(ft_face_);
    FT_Done_FreeType(ft_library_);
}

mfl::math_constants MFLFontFace::constants() const { return {}; }
mfl::math_glyph_info MFLFontFace::glyph_info(const size_t /*glyph_index*/) const { return {}; }
size_t MFLFontFace::glyph_index_from_code_point(const mfl::code_point /*char_code*/, const bool /*use_large_variant*/) const { return 0; }
std::vector<mfl::size_variant> MFLFontFace::horizontal_size_variants(const mfl::code_point /*char_code*/) const { return {}; }
std::vector<mfl::size_variant> MFLFontFace::vertical_size_variants(const mfl::code_point /*char_code*/) const { return {}; }
std::optional<mfl::glyph_assembly> MFLFontFace::horizontal_assembly(const mfl::code_point /*char_code*/) const { return std::nullopt; }
std::optional<mfl::glyph_assembly> MFLFontFace::vertical_assembly(const mfl::code_point /*char_code*/) const { return std::nullopt; }
void MFLFontFace::set_size(const mfl::points /*size*/) {}
