#pragma once

#include <string>
#include <string_view>

// Text that is shown differently from how it is spoken: a proper noun spelled out for TTS, an acronym
// pronounced differently than it reads, or on-screen text that should not be narrated at all.
//
// Syntax, inside a line of script text: <written> or <written|spoken>.
//
//     We built <C++|see plus plus> in a weekend.
//
// `written` is always shown (and becomes the subtitle text). `spoken`, when given, is what the TTS says
// instead of `written`. Without it, `written` is shown but not spoken at all - a gap in the narration, for
// on-screen text that has nothing to say out loud.
//
// A doubled delimiter is the literal character: "<<" is "<", ">>" is ">", "||" is "|". A "<" with no
// matching ">" is left as ordinary text (so "a < b" is unaffected).
struct AltText
{
	std::string written; // for display and subtitles
	std::string spoken;  // for TTS synthesis and alignment
};

inline AltText parse_alt_text( std::string_view text )
{
	AltText out;
	size_t i = 0;
	while ( i < text.size() )
	{
		const char c = text[i];
		if ( ( c == '<' || c == '>' || c == '|' ) && i + 1 < text.size() && text[i + 1] == c )
		{
			out.written += c;
			out.spoken += c;
			i += 2;
			continue;
		}

		if ( c == '<' )
		{
			const size_t pipe = text.find( '|', i + 1 );
			const size_t close = text.find( '>', i + 1 );
			if ( close == std::string_view::npos )
			{
				// no matching '>': not a tag after all
				out.written += c;
				out.spoken += c;
				++i;
				continue;
			}

			if ( pipe != std::string_view::npos && pipe < close )
			{
				out.written += text.substr( i + 1, pipe - ( i + 1 ) );
				out.spoken += text.substr( pipe + 1, close - ( pipe + 1 ) );
			}
			else
			{
				// <written> alone: shown, not spoken
				out.written += text.substr( i + 1, close - ( i + 1 ) );
			}
			i = close + 1;
			continue;
		}

		out.written += c;
		out.spoken += c;
		++i;
	}

	// An unspoken span can leave a doubled space, or one at either end, in the spoken text
	std::string squeezed;
	bool space = false;
	for ( char c: out.spoken )
	{
		if ( c == ' ' )
		{
			space = true;
			continue;
		}
		if ( space && !squeezed.empty() )
		{
			squeezed += ' ';
		}
		space = false;
		squeezed += c;
	}
	out.spoken = std::move( squeezed );

	return out;
}

// The written text alone (display/subtitles); markers ({name}, see utils/Markers.h) are untouched.
inline std::string written_text( std::string_view text )
{
	return parse_alt_text( text ).written;
}
