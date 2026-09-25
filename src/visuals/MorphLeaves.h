#pragma once

#include "visuals/Outline.h"

#include <vector>

// How the leaves (glyphs, shapes) of two visuals are paired up to morph into each other
enum class LeafMatch
{
	// Words first (for text laid out by make_styled_text), then tags, then identical glyphs, then what is
	// left by proximity (only leaves near each other and of a similar size, and not parts that only one
	// visual has a name for; the rest fade): what you want for text and formulas, and the default
	AUTO,
	// Only the words of laid-out text: identical words are kept, in reading order, and the words between
	// them pair up in order, letter by letter ("the big cat" -> "the cat" keeps the and cat, and lets big
	// fade; "one two" -> "uno dos" turns each word into the one at its place)
	BY_WORD,
	// Only leaves under the same tag name (find_tag names: "numerator", "word3", ...), in order
	BY_TAG,
	// Only identical leaves (the same glyph at the same size, a rectangle of the same size ...), lined
	// up in reading order so the rest can slide into place
	BY_GLYPH,
	// The i-th leaf with the i-th leaf
	BY_INDEX
};

// A leaf of the first visual and one of the second (indices into their to_outline() lists), or -1 for
// none: a leaf with no partner fades out, or in
struct LeafPair
{
	int a = -1;
	int b = -1;

	bool operator==( const LeafPair & ) const = default;
};

// Every leaf of either visual appears exactly once. Ordered for drawing: by the second visual's order,
// with leaves that only exist in the first placed before the ones that follow them there.
std::vector<LeafPair> match_leaves( const std::vector<OutlineShape> &a, const std::vector<OutlineShape> &b, LeafMatch mode = LeafMatch::AUTO );
