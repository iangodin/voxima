#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <memory>

using FontFacePtr = std::shared_ptr<std::remove_pointer_t<FT_Face>>;

struct FontFace
{
	FontFacePtr face;
	float scale = 1.F;
};

struct GlyphEntry
{
	size_t font_index;
	uint32_t glyph_index;
};
