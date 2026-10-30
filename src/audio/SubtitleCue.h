
#pragma once

#include <string>

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
