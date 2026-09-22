#pragma once

#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>

std::unique_ptr<Visual> create_math_large_op( const MathFont &font, char32_t op, float target_height );

// The same, stretched left to right instead of top to bottom (braces, wide accents, labeled
// arrows). The result's baseline is at y = 0, its left edge at x = 0.
std::unique_ptr<Visual> create_math_large_op_horizontal( const MathFont &font, char32_t op, float target_width );
