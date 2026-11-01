#pragma once

#include "Theme.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

// Themes written as files (HUML): the fonts of a look, as named font families put in stacks, and which palette it starts with.
// The built-in themes are such files (themes/*.huml, compiled in), and a theme of your own can start from one and change
// what it likes.
//
//     meta::
//       name: "My Course"
//       inherit: "Modern Technical"     # a built-in theme, or a file: <name>.huml in a theme directory
//
//     palette: "github-light"           # the colors it starts with, whole - background included (see Palette)
//
//     palette::                         # or the colors themselves, "#RRGGBB": all eleven (background, text, muted and the
//       inherit: "one-dark"             # eight accents), or only those that differ from a palette it inherits
//       background: "#101820"
//       blue: "#4FB3FF"
//
//     fonts::                           # families (see FontFamily): what a style becomes a file through
//       plex_mono::
//         regular: "IBMPlexMono-Regular.otf"
//         bold: "IBMPlexMono-Bold.otf"
//
//     stacks::                          # lists of families: what the first has no glyph for, the next may
//       code:: "plex_mono", "...fallbacks"        # "...name" is another stack's families
//
//     intents::                         # which stack each kind of text uses (prose, code, title, label, annotation, caption, math)
//       code: "code"
//
// Overriding is by merging: a dict is merged key by key (so `fonts:: fira:: bold: "..."` changes only that file of the family),
// while a stack or anything else given again replaces what it inherits.
namespace themes
{

// The names of the built-in themes
std::vector<std::string> builtin_names();

// A theme by name: a built-in one, or a file `<name>.huml` in one of the directories. A mistake in a file (an unknown
// key, a family that is not there ...) throws std::runtime_error, saying where.
Theme load( std::string_view name, const std::vector<std::filesystem::path> &directories = {} );

// A theme from the text of a file, whose `inherit` is looked for the same way
Theme from_text( std::string_view source, const std::vector<std::filesystem::path> &directories = {} );

}
