#pragma once

#include "AudioBuffer.h"
#include "Compress.h"
#include "Timeline.h"

#include <vector>

// A language's narration placed on a timeline shared with other languages (see Timeline.h): each segment of
// `natural` (cut where `timeline` says) is copied to where `starts` says it begins, and what it does not fill is
// silence. `starts` has one more entry than there are segments, the last being the total length. Speech that is
// next to added silence is faded over a couple of milliseconds so the join does not click; segments that follow
// each other with nothing added between are copied straight through.
//
// A segment that is longer than the time it is given (a language that may be speeded up, see barrier_lengths) is
// made shorter by compress_segment: its pauses first, then the speech a little. `speech[i]` says where the words
// are in segment i (seconds from its start); `stretch` does the speeding up. Neither is needed if no segment has to
// be made shorter.
struct Retimed
{
	AudioBuffer audio;
	std::vector<TimeMap> maps; // for each segment, where the times in it went (identity if it was only moved)
	std::vector<double> tempos; // for each segment, how much its speech was speeded up (1.0 if not)
};

Retimed retime_audio(
	const AudioBuffer &natural,
	const Timeline &timeline,
	const std::vector<double> &starts,
	const std::vector<std::vector<SpeechSpan>> &speech,
	const Stretcher &stretch,
	const CompressOptions &options = {} );

// The same for segments that only ever need padding
AudioBuffer retime_audio( const AudioBuffer &natural, const Timeline &timeline, const std::vector<double> &starts );
