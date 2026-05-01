
#pragma once

#include "visuals/Group.h"
#include "core/Stage.h"

#include "fonts/TextStyle.h"
#include "fonts/FontStack.h"

#include <string>
#include <vector>
#include <string_view>
#include <memory>

class Glyph;
class FontFace;

struct TextRange
{
	size_t font_index = 0;
	size_t start = 0;
	size_t length = 0;
};

std::vector<TextRange> create_ranges( std::string_view str, const FontStack &font );

void center_group( Group &group );
std::unique_ptr<Glyph> create_glyph( unsigned int glyph_id, const FontFace *font_face );

Group &create_text( Stage &stage, const std::string &text, const TextStyle &style );
Group &create_text( Group &group, Theme &theme, const std::string &text, const TextStyle &style );
