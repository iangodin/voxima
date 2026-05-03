#pragma once

#include "Animation.h"

#include <cmath>

struct EaseBackIn
{
	float overshoot = 1.25f;

	Animation operator()( float &value, float start, float end, int64_t total_frames ) const;
};

struct EaseBackOut
{
    float overshoot = 1.25f;

	Animation operator()( float &value, float start, float end, int64_t total_frames ) const;
};

struct EaseBackInOut
{
    float overshoot = 1.25f;

	Animation operator()( float &value, float start, float end, int64_t total_frames ) const;
};

inline constexpr EaseBackIn ease_back_in{};
inline constexpr EaseBackOut ease_back_out{};
inline constexpr EaseBackInOut ease_back_in_out{};

