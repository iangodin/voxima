
#pragma once

#include <algorithm>
#include <string>
#include <vector>

// A single subtitle cue, timed against the actual output audio stream of one
// language track (see AudioMixer::frames_written() / Narrator - this is what
// anchors start/end to real muxed audio rather than a naive cumulative
// duration estimate).
struct SubtitleCue
{
	double start_seconds;
	double end_seconds;
	std::string text;
};

// The cues in a part of the audio, from `from` to `to` (seconds), timed from its start: what a --start-at and --stop-at keep
inline std::vector<SubtitleCue> cues_between( const std::vector<SubtitleCue> &cues, double from, double to )
{
	std::vector<SubtitleCue> kept;
	for ( const SubtitleCue &cue: cues )
	{
		if ( cue.end_seconds > from && cue.start_seconds < to )
		{
			kept.push_back( { std::max( cue.start_seconds, from ) - from, std::min( cue.end_seconds, to ) - from, cue.text } );
		}
	}
	return kept;
}
