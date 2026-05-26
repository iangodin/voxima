
#pragma once

#include "Animation.h"

#include <cmath>

struct EaseElasticIn
{
	float amplitude = 0.25F;
	float period = 0.3F;

	Animation operator()( float start, float end, int64_t total_frames ) const;
};

struct EaseElasticOut
{
    float amplitude = 0.25F;
    float period = 0.3F;

	Animation operator()( float start, float end, int64_t total_frames ) const;
};

struct EaseElasticInOut
{
    float amplitude = 0.25F;
    float period = 0.3F;

	Animation operator()( float start, float end, int64_t total_frames ) const;
};

inline constexpr EaseElasticIn ease_elastic_in{};
inline constexpr EaseElasticOut ease_elastic_out{};
inline constexpr EaseElasticInOut ease_elastic_in_out{};

