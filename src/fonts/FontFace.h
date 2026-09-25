#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <memory>

using FontFacePtr = std::shared_ptr<std::remove_pointer_t<FT_Face>>;

struct FontFace
{
	FontFacePtr face;
	float scale = 1.F;

	// A catch-all font (Adobe NotDef) that draws a plain box for every character, so it always has a glyph. It
	// ends a font stack; a character only it has is one that is really missing (see MissingGlyph.h).
	bool notdef = false;
};

struct GlyphEntry
{
	size_t font_index;
	uint32_t glyph_index;
};
