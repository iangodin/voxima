
#pragma once

#include <cmath>

// Eases that spring: they swing past the end (out), before the start (in), or both, settling like a plucked
// string. `amplitude` is how far, and `period` how quick the swings are; ease_elastic_in and the like are these with
// the defaults.

struct EaseElasticIn
{
	float amplitude = 0.25F;
	float period = 0.3F;

	float operator()( float t ) const;
};

struct EaseElasticOut
{
	float amplitude = 0.25F;
	float period = 0.3F;

	float operator()( float t ) const;
};

struct EaseElasticInOut
{
	float amplitude = 0.25F;
	float period = 0.3F;

	float operator()( float t ) const;
};

inline constexpr EaseElasticIn ease_elastic_in{};
inline constexpr EaseElasticOut ease_elastic_out{};
inline constexpr EaseElasticInOut ease_elastic_in_out{};

