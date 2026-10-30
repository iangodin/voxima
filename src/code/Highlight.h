#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Syntax highlighting of source code, with tree-sitter: the source is parsed, and the grammar's highlight query
// names the parts (keyword, string, function, comment ...).
namespace code
{

enum class Language
{
	C,
	CPP
};

// "c", "cpp" / "c++" / "cxx" (any case), or nothing for a language there is no grammar for
std::optional<Language> language_named( std::string_view name );

// A stretch of the source, in bytes, and what it is: the query's capture name, such as "keyword",
// "string", "function" or "function.special" (a dotted name is a kind of the name before the dot)
struct Span
{
	size_t start = 0;
	size_t end = 0;
	std::string name;

	bool operator==( const Span & ) const = default;
};

// The highlighted parts of `source`, in order, never overlapping (the language first, where it can be seen: the source
// is often a long string over several lines); what is not in any is plain. Where the query
// names a part more than once, a smaller part inside a bigger one wins, and between two names for the same part the
// one from the later pattern wins (queries go from the general to the particular: every identifier is a
// variable, then some identifiers are functions). A C++ source is highlighted with C's query and then C++'s.
std::vector<Span> highlight( Language language, std::string_view source );

}
