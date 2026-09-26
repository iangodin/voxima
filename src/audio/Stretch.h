#pragma once

#include "AudioBuffer.h"

#include <functional>

// Speed audio up (or slow it down) without changing its pitch. `tempo` 1.25 makes it a fifth shorter; 1.0 leaves it
// alone.
using Stretcher = std::function<AudioBuffer( const AudioBuffer &audio, double tempo )>;

// Done by ffmpeg's `atempo` filter (WSOLA), which must be installed: the audio is written to a temporary WAV
// file, run through it, and read back. Throws (saying so) if ffmpeg cannot be run. Good for speech up to
// about 1.25.
AudioBuffer stretch_with_ffmpeg( const AudioBuffer &audio, double tempo );
