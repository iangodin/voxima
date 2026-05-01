#pragma once

#include "visuals/Group.h"
#include "core/Stage.h"

#include "fonts/TextStyle.h"

#include <string>

Group &create_math( Stage &stage, const std::string &formula, const TextStyle &style );
Group &create_math( Group &group, Theme &theme, const std::string &formula, const TextStyle &style );
