
#pragma once

#include "Group.h"
#include "Stage.h"
#include "TextStyle.h"

#include <string>

Group &create_text( Stage &stage, const std::string &text, const TextStyle &style );

