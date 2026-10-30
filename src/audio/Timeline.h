#pragma once

#include "TimedWord.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

// Keeping several languages' narration in step. Each language says the same script in a different time, but the
// script has places that correspond in all of them: the markers written into the text, and the start of each
// line. Cut the narration at those places and every language has the same segments, each of a different length.
// Give each segment the length of the longest one (it is the barrier the languages meet at), start the segments at
// the same moments, and pad the shorter languages with silence at the end of a segment: a marker then falls
// at the same time in every language, and one video can go with all the audio tracks.

// How much of the gap before a cue the cut is made ahead of it, at most: cutting exactly at the start of a
// word would clip its first sound if the alignment is a little late. (So the languages' cues can differ by up
// to this: under a frame.)
inline constexpr double CUT_LEAD_SECONDS = 0.04;

struct TimelineCut
{
	std::string id;      // a marker's name, or a line ("\nstart", "\nline2"; a marker's name cannot hold a newline)
	double seconds = 0.0; // where the audio is cut
};

// One language's narration divided into segments. cuts[0] is the start, at 0; segment i runs from cuts[i] to
// cuts[i + 1] (the last to `end`).
struct Timeline
{
	std::vector<TimelineCut> cuts;
	double end = 0.0;

	size_t segments() const
	{
		return cuts.size();
	}

	double segment_start( size_t i ) const
	{
		return cuts[i].seconds;
	}

	double segment_end( size_t i ) const
	{
		return i + 1 < cuts.size() ? cuts[i + 1].seconds : end;
	}

	double segment_length( size_t i ) const
	{
		return std::max( 0.0, segment_end( i ) - segment_start( i ) );
	}
};

// Whether two languages are cut in the same places, in the same order (they have to be to be kept in step)
inline bool same_structure( const Timeline &a, const Timeline &b )
{
	if ( a.cuts.size() != b.cuts.size() )
	{
		return false;
	}
	for ( size_t i = 0; i < a.cuts.size(); ++i )
	{
		if ( a.cuts[i].id != b.cuts[i].id )
		{
			return false;
		}
	}
	return true;
}

// How long each segment is on the shared timeline: as long as it takes the slowest language. The timelines must
// have the same structure.
inline std::vector<double> barrier_lengths( const std::vector<const Timeline *> &timelines )
{
	std::vector<double> lengths;
	if ( timelines.empty() )
	{
		return lengths;
	}
	lengths.assign( timelines.front()->segments(), 0.0 );
	for ( const Timeline *t: timelines )
	{
		for ( size_t i = 0; i < lengths.size(); ++i )
		{
			lengths[i] = std::max( lengths[i], t->segment_length( i ) );
		}
	}
	return lengths;
}

// The same, when the other languages may be speeded up: a language's segment can be made shorter by up to
// `max_speedup` (1.0: not at all), and the main language never is. A segment is as long as the main language
// takes, or as long as it takes the others at their fastest, whichever is more. `main_index` says which of the
// timelines is the main language's.
inline std::vector<double> barrier_lengths( const std::vector<const Timeline *> &timelines, size_t main_index, double max_speedup )
{
	std::vector<double> lengths;
	if ( timelines.empty() )
	{
		return lengths;
	}
	const double speedup = std::max( 1.0, max_speedup );
	lengths.assign( timelines.front()->segments(), 0.0 );
	for ( size_t t = 0; t < timelines.size(); ++t )
	{
		for ( size_t i = 0; i < lengths.size(); ++i )
		{
			const double length = timelines[t]->segment_length( i );
			lengths[i] = std::max( lengths[i], t == main_index ? length : length / speedup );
		}
	}
	return lengths;
}

// Where each segment starts on the shared timeline; one more entry than there are segments, the last the total
inline std::vector<double> segment_starts( const std::vector<double> &lengths )
{
	std::vector<double> starts{ 0.0 };
	for ( double length: lengths )
	{
		starts.push_back( starts.back() + length );
	}
	return starts;
}

// Where a time in the language's own narration falls on the shared timeline: its place in its segment, from that
// segment's new start
inline double retimed( const Timeline &timeline, const std::vector<double> &starts, double seconds )
{
	size_t i = 0;
	while ( i + 1 < timeline.segments() && timeline.segment_start( i + 1 ) <= seconds )
	{
		++i;
	}
	return starts[i] + ( seconds - timeline.segment_start( i ) );
}

// How a time inside a segment moves when the segment is shortened: a piecewise linear map from time in the
// segment as it was (from its start) to time in it as it is now (also from its start). A segment that is only
// moved has none (the identity).
class TimeMap
{
public:
	TimeMap() = default;

	// Pairs of (was, is), in increasing order. Before the first and after the last the map goes on at the same
	// speed as time.
	explicit TimeMap( std::vector<std::pair<double, double>> knots ) :
		_knots( std::move( knots ) )
	{
	}

	bool identity() const
	{
		return _knots.empty();
	}

	double operator()( double was ) const
	{
		if ( _knots.empty() )
		{
			return was;
		}
		if ( was <= _knots.front().first )
		{
			return _knots.front().second + ( was - _knots.front().first );
		}
		for ( size_t i = 1; i < _knots.size(); ++i )
		{
			if ( was <= _knots[i].first )
			{
				const double span = _knots[i].first - _knots[i - 1].first;
				if ( span <= 0.0 )
				{
					return _knots[i].second;
				}
				const double along = ( was - _knots[i - 1].first ) / span;
				return _knots[i - 1].second + along * ( _knots[i].second - _knots[i - 1].second );
			}
		}
		return _knots.back().second + ( was - _knots.back().first );
	}

private:
	std::vector<std::pair<double, double>> _knots;
};

// Where a time in the language's own narration falls on the shared timeline when its segments may have been
// shortened: its place in its segment, moved through that segment's map, from the segment's new start
inline double retimed( const Timeline &timeline, const std::vector<double> &starts, const std::vector<TimeMap> &maps, double seconds )
{
	size_t i = 0;
	while ( i + 1 < timeline.segments() && timeline.segment_start( i + 1 ) <= seconds )
	{
		++i;
	}
	return starts[i] + maps[i]( seconds - timeline.segment_start( i ) );
}

// One segment's audio: copied from `from` to `to` in the language's own narration to `dest` on the shared timeline
struct Piece
{
	double from;
	double to;
	double dest;
};

inline std::vector<Piece> pieces( const Timeline &timeline, const std::vector<double> &starts )
{
	std::vector<Piece> out;
	for ( size_t i = 0; i < timeline.segments(); ++i )
	{
		out.push_back( { timeline.segment_start( i ), timeline.segment_end( i ), starts[i] } );
	}
	return out;
}

// Where to cut for a marker at `offset` in a line whose words are `words`, and which is reached at `seconds` (see
// time_at_offset): a little before the word it comes in front of, but no more than half the gap before it (the pause
// where the cut is made), and not into a word.
inline double marker_cut( const std::vector<TimedWord> &words, size_t offset, double seconds, double line_start )
{
	double previous_end = line_start;
	for ( const auto &w: words )
	{
		if ( offset <= w.begin )
		{
			const double gap = seconds - previous_end;
			return seconds - std::clamp( gap / 2.0, 0.0, CUT_LEAD_SECONDS );
		}
		if ( offset < w.end )
		{
			return seconds; // inside a word
		}
		previous_end = w.end_seconds;
	}
	return seconds; // after the last word
}
