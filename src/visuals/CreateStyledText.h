#pragma once

#include "visuals/Group.h"
#include "visuals/StyledText.h"

#include <memory>

struct Theme;

/// Lay out styled text as a group of TextLines. The origin is the start of the first
/// line's baseline (baseline at y = 0); the result is not centered.
std::unique_ptr<Group> make_styled_text( Theme &theme, const StyledText &text );
