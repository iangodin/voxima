
#pragma once

#include <string>
#include <string_view>
#include <vector>

// Minimal BCP-47 tag handling - just enough for the tag shapes this codebase
// actually uses (a primary subtag, optionally followed by one or more
// hyphen-separated subtags, e.g. "en", "en-GB", "zh-Hans-CN"). Not a full
// RFC 5646 grammar/validator.
namespace language
{

// Normalized (lowercased) primary language subtag, e.g. "en-GB" -> "en",
// "zh-Hans-CN" -> "zh", "en" -> "en". This is the ISO 639-1 code TTS language
// allowlists want, as opposed to the full regional/script tag.
std::string primary_subtag( std::string_view tag );

// The three-letter ISO 639-2 code of a tag's language, which is what containers label their tracks with: "en-GB" -> "eng". mp4 takes
// the terminology form, and Matroska the bibliographic one, which differ for a few languages ("deu" and "ger"). A language that is
// not known is its primary subtag, as it is.
std::string container_language( std::string_view tag, bool bibliographic );

// Fallback chain from most to least specific: the tag itself first (used
// verbatim, preserving whatever case it was given, since it may also be a
// lookup key elsewhere), then its normalized primary subtag if that differs.
// "en-GB" -> {"en-GB", "en"}; "en" -> {"en"}.
std::vector<std::string> fallback_chain( std::string_view tag );

}
