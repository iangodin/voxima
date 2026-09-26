#pragma once

#include "AudioBuffer.h"
#include "Timeline.h"

#include <vector>

// A language's narration placed on a timeline shared with other languages (see Timeline.h): each segment of
// `natural` (cut where `timeline` says) is copied to where `starts` says it begins, and what it does not fill is
// silence. `starts` has one more entry than there are segments, the last being the total length. Speech that is
// next to added silence is faded over a couple of milliseconds so the join does not click; segments that follow
// each other with nothing added between are copied straight through.
AudioBuffer retime_audio( const AudioBuffer &natural, const Timeline &timeline, const std::vector<double> &starts );
