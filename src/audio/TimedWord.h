#pragma once

#include <utils/Words.h>

#include <algorithm>

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <vector>

// One narrated word, timed in seconds relative to the start of a narration
// batch (i.e. the first sample of the first line passed to Narrator::speech).
// Words come from split_words(), the same splitter the text layout uses, so
// the n-th word of a line here is the n-th word of that line on screen.
struct TimedWord
{
	std::string word;
	size_t line = 0;  // index of the narrated line this word belongs to
	size_t begin = 0; // byte range of the word within that line's text
	size_t end = 0;
	double start_seconds = 0.0;
	double end_seconds = 0.0;
};

// Spreads the words of `text` across [start_seconds, start_seconds +
// duration_seconds), each word getting a share proportional to its length in
// UTF-8 code points. Used when forced alignment is unavailable (e.g. Polish),
// fails, or disagrees with split_words() about the word count. Words abut
// with no gaps, so this is coarser than real alignment but keeps the
// animation roughly in sync.
inline std::vector<TimedWord> distribute_words(
	std::string_view text, size_t line, double start_seconds, double duration_seconds )
{
	const auto spans = split_words( text );

	size_t total = 0;
	for ( auto span: spans )
	{
		total += codepoint_count( text.substr( span.begin, span.end - span.begin ) );
	}

	std::vector<TimedWord> words;
	size_t consumed = 0;
	for ( auto span: spans )
	{
		const auto word = text.substr( span.begin, span.end - span.begin );
		const double a =
			start_seconds + duration_seconds * static_cast<double>( consumed ) / static_cast<double>( total );
		consumed += codepoint_count( word );
		const double b =
			start_seconds + duration_seconds * static_cast<double>( consumed ) / static_cast<double>( total );
		words.push_back( TimedWord{ std::string( word ), line, span.begin, span.end, a, b } );
	}
	return words;
}

// Pairs the aligner's per-word times with the words of `text`. Returns empty
// when the aligner's word count differs from split_words()'s (it tokenizes
// differently around numbers, hyphens, ...), in which case the caller should
// fall back to distribute_words().
template<typename Aligned>
std::vector<TimedWord> match_words(
	std::string_view text, size_t line, double offset_seconds, const std::vector<Aligned> &aligned )
{
	const auto spans = split_words( text );
	if ( spans.size() != aligned.size() )
	{
		return {};
	}

	std::vector<TimedWord> words;
	for ( size_t i = 0; i < spans.size(); ++i )
	{
		words.push_back( TimedWord{
			std::string( text.substr( spans[i].begin, spans[i].end - spans[i].begin ) ),
			line,
			spans[i].begin,
			spans[i].end,
			offset_seconds + aligned[i].start_seconds,
			offset_seconds + aligned[i].end_seconds } );
	}
	return words;
}

// How far `position` is through [from, to], from 0 (at or before from) to 1 (at or past to). A span with
// no length is 1 once it has been reached.
inline float progress( double position, double from, double to )
{
	if ( to <= from )
	{
		return position >= from ? 1.F : 0.F;
	}
	const double t = ( position - from ) / ( to - from );
	return static_cast<float>( t < 0.0 ? 0.0 : ( t > 1.0 ? 1.0 : t ) );
}

// The time of a marker: a cue point at `offset` in a line's text (a byte offset, as in Marker) is when the speech
// gets there. Before a word, that is when the word starts (so a marker right after a word, before the space,
// is the start of the next one: when the speech picks up again); inside a word, how far through it in letters;
// after the last word, when it ends. `words` are the words of that one line; `fallback` is for a line with none.
inline double time_at_offset( const std::vector<TimedWord> &words, size_t offset, double fallback )
{
	if ( words.empty() )
	{
		return fallback;
	}
	for ( const auto &w: words )
	{
		if ( offset <= w.begin )
		{
			return w.start_seconds;
		}
		if ( offset < w.end )
		{
			const size_t total = codepoint_count( w.word );
			const size_t before = codepoint_count( std::string_view( w.word ).substr( 0, offset - w.begin ) );
			return w.start_seconds + ( w.end_seconds - w.start_seconds ) * static_cast<double>( before ) / static_cast<double>( total );
		}
	}
	return words.back().end_seconds;
}

// A marker with the time it is reached in a narration batch (seconds since it started)
struct TimedMarker
{
	std::string name;
	double seconds = 0.0;
	size_t line = 0; // the narrated line it is in
};

// Every language should have the same markers, in the same order, as the main one. What is wrong, one problem per line
// (nothing if all is well). `names` maps a language to the names of its markers in order.
inline std::vector<std::string> marker_mismatches( const std::map<std::string, std::vector<std::string>> &names, const std::string &main_language )
{
	auto list = []( const std::vector<std::string> &items )
	{
		std::string out = "[";
		for ( size_t i = 0; i < items.size(); ++i )
		{
			out += ( i ? " " : "" ) + items[i];
		}
		return out + "]";
	};

	std::vector<std::string> problems;
	const auto main_it = names.find( main_language );
	if ( main_it == names.end() )
	{
		return problems;
	}

	for ( const auto &[language, markers]: names )
	{
		if ( language == main_language || markers == main_it->second )
		{
			continue;
		}

		std::string message = language + " has markers " + list( markers ) + " but " + main_language + " has " + list( main_it->second );
		std::string missing;
		for ( const auto &name: main_it->second )
		{
			if ( std::find( markers.begin(), markers.end(), name ) == markers.end() )
			{
				missing += ( missing.empty() ? "" : " " ) + name;
			}
		}
		std::string extra;
		for ( const auto &name: markers )
		{
			if ( std::find( main_it->second.begin(), main_it->second.end(), name ) == main_it->second.end() )
			{
				extra += ( extra.empty() ? "" : " " ) + name;
			}
		}
		if ( !missing.empty() )
		{
			message += "; missing: " + missing;
		}
		if ( !extra.empty() )
		{
			message += "; not in " + main_language + ": " + extra;
		}
		if ( missing.empty() && extra.empty() )
		{
			message += "; same markers in a different order";
		}
		problems.push_back( message );
	}
	return problems;
}
