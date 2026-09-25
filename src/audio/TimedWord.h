#pragma once

#include <utils/Words.h>

#include <cstddef>
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
