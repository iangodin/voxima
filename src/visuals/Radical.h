#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "visuals/Rectangle.h"
#include "fonts/MathFont.h"
#include "fonts/MathStyle.h"
#include <memory>

class Radical : public Group
{
public:
    // Square root of `content`.
    Radical(
        const MathFont &font,
        float glyph_scale,
        std::unique_ptr<Visual> &&content,
        MathStyle style = MathStyle::display());

    // Root of `content` with a degree (e.g. the 3 of a cube root). The caller builds the
    // index at the size it wants; TeX sets it at about half the body size (the font's
    // scriptScriptPercentScaleDown).
    Radical(
        const MathFont &font,
        float glyph_scale,
        std::unique_ptr<Visual> &&index,
        std::unique_ptr<Visual> &&content,
        MathStyle style = MathStyle::display());
};
