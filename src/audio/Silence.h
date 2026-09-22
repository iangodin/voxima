#pragma once

#include "AudioBuffer.h"

/// @brief Create a buffer of silence
/// @param frames Number of frames
/// @param rate Sample rate
/// @param channels Number of channels
/// @return A new AudioBuffer filled with silence
AudioBuffer make_silence( size_t frames, size_t rate, size_t channels );

/// @brief Create a buffer of silence for a given duration
/// @param duration Duration in seconds
/// @param rate Sample rate
/// @param channels Number of channels
/// @return A new AudioBuffer filled with silence
AudioBuffer make_silence_for_duration( double duration, size_t rate, size_t channels );
