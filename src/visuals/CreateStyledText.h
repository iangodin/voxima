#pragma once

#include "visuals/Group.h"
#include "visuals/StyledText.h"

#include <memory>

struct Theme;

enum class TextAlign
{
	LEFT,
	CENTER,
	RIGHT
};

/// How the lines of a text block sit relative to each other.
struct TextLayout
{
	/// Lines are aligned within the width of the widest line
	TextAlign align = TextAlign::LEFT;

	/// Baseline-to-baseline distance, as a multiple of the largest font size on the line
	/// above (a blank line counts as the line before it)
	float line_spacing = 1.2F;
};

/// Lay out styled text as a group of TextLines, each holding its words as groups tagged
/// "word0", "word1", ... (counting through the whole text, split by split_words()). The
/// origin is the start of the first line's baseline (baseline at y = 0) and the left edge
/// of the block; the result is not centered.
std::unique_ptr<Group> make_styled_text( Theme &theme, const StyledText &text, const TextLayout &layout = {} );

class Glyph;

/// A glyph of laid-out text, and where in the text it came from
struct PlacedGlyphInfo
{
	size_t offset = 0; // byte offset of its character (the first, for a ligature) in StyledText::text()
	Glyph *glyph = nullptr;
};

/// The same, laid out into `root` (added to its children), and telling which glyph is which character: every
/// glyph, spaces too, in the order they are laid out (for right-to-left text, not the order of the text). The
/// pointers are to glyphs now owned by `root`.
void make_styled_text( Group &root, Theme &theme, const StyledText &text, const TextLayout &layout, std::vector<PlacedGlyphInfo> *glyphs );
