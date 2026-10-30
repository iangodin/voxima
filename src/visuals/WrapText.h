#pragma once

#include "visuals/CreateText.h"

#include <string>

// `text` broken into lines no wider than `max_width` (stage units) when set in `style`, at the spaces between words: the
// string with a newline in place of each space where a line ends. A word wider than the width is left on a line of its own.
// (Only for languages that put spaces between words.) Newlines already in the text end a line as they do.
std::string wrap_lines( Theme &theme, const std::string &text, const FontStyle &style, float max_width, const std::optional<std::string> &language = std::nullopt );

// make_text of the text wrapped to a width: a paragraph
std::unique_ptr<Group> make_paragraph(
	Theme &theme,
	const std::string &text,
	const FontStyle &style,
	float max_width,
	const std::optional<std::string> &language = std::nullopt,
	const TextLayout &layout = {} );
