#pragma once

#include "visuals/Group.h"
#include "core/Stage.h"
#include "visuals/StyledText.h"

#include <memory>

class Group;
class Stage;
class StyledText;

Group &create_styled_text( Stage &stage, const StyledText &text );
Group &create_styled_text( Stage &stage, Group &group, const StyledText &text );
