#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

// How words are said, from a script's `pronounce` section: what is written, and what the TTS says for it.
using Pronunciations = std::vector<std::pair<std::string, std::string>>;

// Text with the words in the list said as the list says, for the TTS. It goes by whole words, so "HLG" is not found in "HLGs", and
// case matters. Where more than one could apply the longest is used, and what has been put in is not looked at again.
// Markers ({name}, see utils/Markers.h) are left as they are.
inline std::string apply_pronunciations( std::string_view text, const Pronunciations &list )
{
	if ( list.empty() )
	{
		return std::string( text );
	}
	// Letters and digits; anything past ASCII is taken to be a letter
	const auto in_word = []( char c )
	{
		const auto u = static_cast<unsigned char>( c );
		return u >= 0x80 || ( u >= '0' && u <= '9' ) || ( u >= 'a' && u <= 'z' ) || ( u >= 'A' && u <= 'Z' );
	};
	const auto matches = [&]( size_t at, std::string_view word )
	{
		if ( word.empty() || text.substr( at, word.size() ) != word )
		{
			return false;
		}
		if ( in_word( word.front() ) && at > 0 && in_word( text[at - 1] ) )
		{
			return false;
		}
		const size_t after = at + word.size();
		return !( in_word( word.back() ) && after < text.size() && in_word( text[after] ) );
	};

	std::string out;
	size_t i = 0;
	while ( i < text.size() )
	{
		if ( text[i] == '{' )
		{
			// A literal brace, or a marker (letters, digits, '_' and '-' in braces; other braces are ordinary text): copied as it is
			if ( i + 1 < text.size() && text[i + 1] == '{' )
			{
				out += "{{";
				i += 2;
				continue;
			}
			size_t close = i + 1;
			while ( close < text.size() && ( ( in_word( text[close] ) && static_cast<unsigned char>( text[close] ) < 0x80 ) || text[close] == '_' || text[close] == '-' ) )
			{
				++close;
			}
			if ( close > i + 1 && close < text.size() && text[close] == '}' )
			{
				out += text.substr( i, close + 1 - i );
				i = close + 1;
				continue;
			}
		}
		const std::pair<std::string, std::string> *best = nullptr;
		for ( const auto &entry: list )
		{
			if ( ( !best || entry.first.size() > best->first.size() ) && matches( i, entry.first ) )
			{
				best = &entry;
			}
		}
		if ( best )
		{
			out += best->second;
			i += best->first.size();
		}
		else
		{
			out += text[i];
			++i;
		}
	}
	return out;
}
