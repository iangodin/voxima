
#pragma once

#include "Animation.h"

#include <cmath>

Animation ease_in_elastic( float &value, float start, float end, int64_t total_frames );
Animation ease_out_elastic( float &value, float start, float end, int64_t total_frames );
Animation ease_in_out_elastic( float &value, float start, float end, int64_t total_frames );

