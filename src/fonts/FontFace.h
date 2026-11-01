#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

// FreeType's face (FT_Face is a pointer to it). Only Voxima's own code looks inside, so a plugin does not need FreeType.
struct FT_FaceRec_;

using FontFacePtr = std::shared_ptr<FT_FaceRec_>;

struct FontFace
{
	FontFacePtr face;
	float scale = 1.F;

	// A catch-all font (Adobe NotDef) that draws a plain box for every character, so it always has a glyph. It
	// ends a font stack; a character only it has is one that is really missing (see MissingGlyph.h).
	bool notdef = false;

	// Where it came from, for saying what a font stack holds (and testing it): the file and face, and the variable font's
	// axes: "NewCM10-Regular.otf#0 w400 wd100 sl0"
	std::string source;
};

struct GlyphEntry
{
	size_t font_index;
	uint32_t glyph_index;
};
