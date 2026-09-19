#pragma once

#include "visuals/Glyph.h"

// A glyph taken from a math font's large-operator variants and assemblies. Its italic
// correction and top accent attachment live on Glyph, like every other glyph's.
struct MathGlyph : public Glyph
{
};
