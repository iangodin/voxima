#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "visuals/Rectangle.h"
#include "fonts/MathFont.h"
#include <memory>

class Radical : public Group
{
public:
    Radical(const MathFont &font, float glyph_scale, std::unique_ptr<Visual> &&content);
};
