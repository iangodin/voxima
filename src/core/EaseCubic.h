
#pragma once

#include "Animation.h"

#include <cmath>

Animation ease_in_cubic( float &value, float start, float end, int64_t total_frames );
Animation ease_out_cubic( float &value, float start, float end, int64_t total_frames );
Animation ease_in_out_cubic( float &value, float start, float end, int64_t total_frames );

