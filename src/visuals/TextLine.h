#pragma once

#include "Group.h"

// A single laid-out line of text. `baseline` and `math_axis` (inherited from
// Visual) are filled in by make_styled_text() as the line is shaped.
class TextLine : public Group
{
};
