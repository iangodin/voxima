
#pragma once

#include "Group.h"
#include "Stage.h"
#include "TextStyle.h"
#include "FontStack.h"

#include <string>
#include <vector>
#include <string_view>

struct TextRange
{
	size_t font_index = 0;
	size_t start = 0;
	size_t length = 0;
};

std::vector<TextRange> create_ranges( std::string_view str, const FontStack &font );

Group &create_text( Stage &stage, const std::string &text, const TextStyle &style );
