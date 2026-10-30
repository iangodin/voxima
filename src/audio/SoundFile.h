#pragma once

#include "AudioBuffer.h"

#include <filesystem>

// Read a sound file (WAV, FLAC, Ogg Vorbis/Opus, MP3: whatever libsndfile reads) into memory as it is: its own
// rate and channels. Throws if it cannot be read.
AudioBuffer read_sound( const std::filesystem::path &path );

// The same sound in two channels: mono is copied to both, stereo is left as it is, and more channels are
// folded down (the first two kept). An empty buffer stays empty.
AudioBuffer to_stereo( const AudioBuffer &audio );

// A sound ready to mix: read, made stereo and resampled to `rate`
AudioBuffer load_sound( const std::filesystem::path &path, size_t rate );
