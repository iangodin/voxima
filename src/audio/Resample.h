#pragma once

#include "AudioBuffer.h"

/// @brief Resample an AudioBuffer to a new rate
/// @param input The input AudioBuffer
/// @param new_rate The target sample rate
/// @return A new AudioBuffer with the resampled data
AudioBuffer resample(const AudioBuffer& input, size_t new_rate);
