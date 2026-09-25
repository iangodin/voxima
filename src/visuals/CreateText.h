
#pragma once

#include <visuals/CreateStyledText.h>
#include <visuals/Group.h>

#include <fonts/FontStyle.h>
#include <fonts/FontStack.h>

#include <optional>
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

/// Lay out text in the given style, baseline at y = 0 (not centered). The math intent
/// sets letters in math italic unless the style says otherwise. language, when given, is
/// a BCP-47 tag (e.g. "en", "fr-CA") passed to Harfbuzz to help shape/layout glyphs.
std::unique_ptr<Group> make_text(
	Theme &theme,
	const std::string &text,
	const FontStyle &style,
	const std::optional<std::string> &language = std::nullopt,
	const TextLayout &layout = {} );
