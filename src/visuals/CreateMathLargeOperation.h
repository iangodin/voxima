#pragma once

#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>

std::unique_ptr<Visual> create_math_large_op( const MathFont &font, char32_t op, float target_height );
