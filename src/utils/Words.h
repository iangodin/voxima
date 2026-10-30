#pragma once

#include "Utf8.h"

#include <cstddef>
#include <string_view>
#include <vector>

// The one definition of "a word" shared by everything that has to agree on
// it: narration timing (audio/TimedWord.h) and text layout (word groups in
// visuals/CreateStyledText.cpp). Keeping a single splitter is what lets the
// n-th spoken word be looked up as the n-th word of the on-screen text.
//
// Rules (a deliberately dictionary-free approximation of UAX #29):
//  - Words are separated by Unicode whitespace; NBSP-style no-break spaces
//    (U+00A0, U+2007, U+202F) glue words together instead.
//  - Punctuation stays attached to its word ("Hello," is one word).
//  - A token with nothing pronounceable in it (a lone "-", an em dash, a
//    quote mark) is not a word. Symbols that are spoken (& % + = @ # $ ...)
//    do count.
//  - Han ideographs, hiragana and katakana have no spaces, and segmenting
//    them properly needs a dictionary, so each character is its own word.
//    The prolonged sound mark, (semi-)voiced marks and the iteration mark
//    attach to the character before them, and adjoining punctuation attaches
//    to its neighbour (quotes forward, stops backward).
struct WordSpan
{
	size_t begin = 0; // byte offset of the first byte of the word
	size_t end = 0;   // byte offset one past the last byte

	bool operator==( const WordSpan & ) const = default;
};

namespace words_detail
{

inline char32_t next_codepoint( std::string_view text, size_t pos, size_t &length )
{
	std::string_view rest = text.substr( pos );
	const size_t before = rest.size();
	try
	{
		const char32_t cp = decode_utf8( rest );
		length = before - rest.size();
		return cp;
	}
	catch ( const std::runtime_error & )
	{
		length = 1; // invalid byte: consume it alone
		return 0xFFFD;
	}
}

inline bool is_space( char32_t cp )
{
	return cp == U' ' || ( cp >= 0x09 && cp <= 0x0D ) || cp == 0x85 || cp == 0x1680 ||
		   ( cp >= 0x2000 && cp <= 0x200B && cp != 0x2007 ) || cp == 0x2028 || cp == 0x2029 || cp == 0x205F ||
		   cp == 0x3000;
}

inline bool is_ideographic( char32_t cp )
{
	return ( cp >= 0x3400 && cp <= 0x4DBF ) || ( cp >= 0x4E00 && cp <= 0x9FFF ) ||
		   ( cp >= 0xF900 && cp <= 0xFAFF ) || ( cp >= 0x20000 && cp <= 0x323AF ) ||
		   ( cp >= 0x3041 && cp <= 0x3096 ) || ( cp >= 0x30A1 && cp <= 0x30FA ) ||
		   ( cp >= 0x30FD && cp <= 0x30FF ) || ( cp >= 0x31F0 && cp <= 0x31FF );
}

// Continues the preceding ideographic word rather than starting its own
inline bool is_attached( char32_t cp )
{
	return cp == 0x30FC || cp == 0x3099 || cp == 0x309A || cp == 0x309B || cp == 0x309C || cp == 0x3005;
}

// Punctuation that is not pronounced
inline bool is_silent( char32_t cp )
{
	switch ( cp )
	{
		case U'!': case U'"': case U'\'': case U'(': case U')': case U',': case U'-': case U'.':
		case U':': case U';': case U'?': case U'[': case U']': case U'{': case U'}': case U'_':
		case U'~': case U'`': case U'^': case U'\\': case U'|':
		case 0xA1: case 0xAB: case 0xBB: case 0xBF:
		case 0x2026: case 0x3001: case 0x3002: case 0xFF01: case 0xFF0C: case 0xFF0E: case 0xFF1A:
		case 0xFF1B: case 0xFF1F:
			return true;
		default:
			return ( cp >= 0x2010 && cp <= 0x2015 ) || ( cp >= 0x2018 && cp <= 0x201F ) ||
				   ( cp >= 0x300C && cp <= 0x300F ) || ( cp >= 0x3008 && cp <= 0x3011 );
	}
}

}

// Byte ranges of the words in `text`, in logical (reading) order.
inline std::vector<WordSpan> split_words( std::string_view text )
{
	using namespace words_detail;

	std::vector<WordSpan> words;

	bool open = false;    // a token is being built
	bool voiced = false;  // ...and it contains something pronounceable
	bool ideo = false;    // ...and it is an ideographic (per-character) token
	size_t begin = 0;

	auto close = [&]( size_t end )
	{
		if ( open && voiced )
		{
			words.push_back( { begin, end } );
		}
		open = voiced = ideo = false;
	};

	size_t pos = 0;
	while ( pos < text.size() )
	{
		size_t length = 0;
		const char32_t cp = next_codepoint( text, pos, length );

		if ( is_space( cp ) )
		{
			close( pos );
		}
		else if ( is_ideographic( cp ) )
		{
			// A silent prefix (an opening quote) joins the character it opens
			if ( open && !voiced && !ideo )
			{
				voiced = true;
				ideo = true;
			}
			else
			{
				close( pos );
				open = voiced = ideo = true;
				begin = pos;
			}
		}
		else if ( ideo )
		{
			if ( is_attached( cp ) || is_silent( cp ) )
			{
				// stays with the character before it
			}
			else
			{
				close( pos );
				open = true;
				voiced = !is_silent( cp );
				begin = pos;
			}
		}
		else
		{
			if ( !open )
			{
				open = true;
				voiced = false;
				begin = pos;
			}
			if ( !is_silent( cp ) )
			{
				voiced = true;
			}
		}

		pos += length;
	}
	close( text.size() );

	return words;
}

// Number of Unicode code points in `text` (UTF-8 continuation bytes skipped).
inline size_t codepoint_count( std::string_view text )
{
	size_t n = 0;
	for ( char c: text )
	{
		n += ( static_cast<unsigned char>( c ) & 0xC0 ) != 0x80;
	}
	return n;
}
