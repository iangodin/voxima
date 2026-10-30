#pragma once

#include "AudioBuffer.h"
#include "Stretch.h"
#include "Timeline.h"

#include <vector>

// Making a stretch of narration shorter without making it sound rushed. People speak faster mostly by pausing less,
// so the pauses go first: each is shortened, down to a floor, by as much as is needed. Only if that is not enough
// is the speech itself sped up, by the small amount left (with a Stretcher: pitch stays the same).

// Where the speech is in a segment: from where a word starts to where it ends, in seconds from the start of the
// segment. Everything else is a pause.
struct SpeechSpan
{
	double start = 0.0;
	double end = 0.0;
};

struct CompressOptions
{
	// A pause is never made shorter than this (seconds): words still need room between them
	double pause_floor = 0.08;
};

struct Compressed
{
	AudioBuffer audio; // exactly as long as asked
	TimeMap map;       // where each time in the original went
	double tempo = 1.0; // how much the speech itself was speeded up (1.0: only pauses were shortened)
};

// `segment` made `target_seconds` long. If it is already that short (or shorter) it is kept as it is, with silence
// after it to fill the time. `speech` says where the words are; with none, the whole segment is a pause.
// Throws if speech has to be speeded up and there is no `stretch`.
Compressed compress_segment(
	const AudioBuffer &segment,
	const std::vector<SpeechSpan> &speech,
	double target_seconds,
	const Stretcher &stretch,
	const CompressOptions &options = {} );
