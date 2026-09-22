#pragma once

#include "AudioBuffer.h"

/// @brief Scale every sample in a buffer by a linear gain factor, in place
/// @param buffer The buffer to modify
/// @param gain Linear gain (1.0 = unchanged, 0.5 = half amplitude, 2.0 = double amplitude)
void apply_volume( AudioBuffer &buffer, float gain );

/// @brief Convert a decibel value to a linear gain factor
/// @param db Gain in decibels (0 = unchanged)
/// @return Linear gain factor
float db_to_gain( float db );
