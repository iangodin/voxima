#pragma once

#include "visuals/Group.h"
#include "core/Stage.h"

#include "fonts/FontStyle.h"

#include <string>

Group &create_math( Stage &stage, const std::string &formula, const FontStyle &style );
Group &create_math( Stage &stage, Group &group, const std::string &formula, const FontStyle &style );
