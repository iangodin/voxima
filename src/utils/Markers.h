#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// Markers are cue points written into a script's text, in every language, at the places that correspond:
//
//     en: These show {a}various ease{b} animations.
//     fr: Ceux-ci montrent {a}diverses animations{b} de lissage.
//
// They are never spoken and never drawn. Each language's narration gets a time for every marker, so whatever is
// animated can be tied to what is being said rather than to which word it is (words differ in number and length
// between languages; a marker means the same thing in all of them).
//
// Syntax: {name}, the name being letters, digits, '_' and '-'. Anything else in braces ("{a, b}") is ordinary text,
// and a doubled brace is a literal one: "{{a}}" is the text "{a}". A marker between two spaces leaves one.
struct Marker
{
	std::string name;
	size_t offset = 0; // byte offset in the text without the markers: it is just before the character at that offset
};

struct MarkedText
{
	std::string text;             // the text with the markers taken out
	std::vector<Marker> markers;  // in the order they appear
};

inline MarkedText parse_markers( std::string_view marked )
{
	auto is_name_char = []( char c )
	{
		return ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) || ( c >= '0' && c <= '9' ) || c == '_' || c == '-';
	};

	MarkedText out;
	size_t i = 0;
	while ( i < marked.size() )
	{
		const char c = marked[i];
		if ( c == '{' )
		{
			if ( i + 1 < marked.size() && marked[i + 1] == '{' )
			{
				out.text += '{'; // an escaped brace
				i += 2;
				continue;
			}

			size_t j = i + 1;
			while ( j < marked.size() && is_name_char( marked[j] ) )
			{
				++j;
			}
			if ( j > i + 1 && j < marked.size() && marked[j] == '}' )
			{
				out.markers.push_back( { std::string( marked.substr( i + 1, j - i - 1 ) ), out.text.size() } );
				i = j + 1;
				// between two spaces it would leave two
				if ( !out.text.empty() && out.text.back() == ' ' && i < marked.size() && marked[i] == ' ' )
				{
					++i;
				}
				continue;
			}
		}
		else if ( c == '}' && i + 1 < marked.size() && marked[i + 1] == '}' )
		{
			out.text += '}'; // an escaped brace
			i += 2;
			continue;
		}

		out.text += c;
		++i;
	}
	return out;
}

// The text without its markers
inline std::string strip_markers( std::string_view marked )
{
	return parse_markers( marked ).text;
}
