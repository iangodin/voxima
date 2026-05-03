
#pragma once

#include "Animation.h"

#include <cmath>

Animation ease_cubic_in( float &value, float start, float end, int64_t total_frames );
Animation ease_cubic_out( float &value, float start, float end, int64_t total_frames );
Animation ease_cubic_in_out( float &value, float start, float end, int64_t total_frames );

