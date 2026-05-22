#pragma once

#include "visuals/MathGlyph.h"
#include "fonts/MathFont.h"
#include <memory>

std::unique_ptr<MathGlyph> create_math_large_op( const MathFont &font, char32_t op, float target_height );
