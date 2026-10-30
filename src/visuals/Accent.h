#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>

// An accent (hat, tilde, bar, dot, vector arrow ...) over a base, placed as TeX's rule 12:
//  - horizontally, the accent's attachment point sits over the base's (each from the MATH
//    table; the centre of the box when the base is several characters or a composite),
//  - vertically, the accent is designed for a base as tall as AccentBaseHeight and is raised
//    by however much taller than that the base is.
// `accent` is the code point of the accent glyph, spacing (U+02C6, U+02DC, U+00AF, U+02D9,
// U+00A8, U+02C7, U+02D8, U+00B4, U+0060) or combining (U+20D7 ...). The base's baseline is at
// y = 0 and its left edge at x = 0, and the box is as wide as the base.
class Accent : public Group
{
public:
	Accent( const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&base, char32_t accent );
};

// An accent that stretches to be as wide as its base (TeX's \widehat, \widetilde, \overrightarrow): the widest of the font's
// variants of the accent that is no wider than the base, or the font's assembly of pieces when it is wider than all of them.
// `accent` is a combining code point (U+0302 hat, U+0303 tilde, U+20D7 right arrow, U+20D6 left arrow ...). Placed like Accent
// otherwise, and as wide as the base.
class WideAccent : public Group
{
public:
	WideAccent( const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&base, char32_t accent );
};
