
#pragma once

#include <visuals/Group.h>

#include <fonts/FontStyle.h>
#include <fonts/FontStack.h>

#include <string>
#include <vector>
#include <string_view>
#include <memory>

class Group;
struct Theme;
struct Glyph;
struct FontFace;

struct TextRange
{
	size_t font_index = 0;
	size_t start = 0;
	size_t length = 0;
};

std::vector<TextRange> create_ranges( std::string_view str, const FontStack &font );

void center_group( Group &group );
std::unique_ptr<Glyph> create_glyph( unsigned int glyph_id, const FontFace *font_face );

/// Lay out plain text in the given style, baseline at y = 0 (not centered).
std::unique_ptr<Group> make_text( Theme &theme, const std::string &text, const FontStyle &style );
