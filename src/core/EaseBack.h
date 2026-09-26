#pragma once

#include <cmath>

// Eases that pull back before going (in), overshoot before settling (out), or both. `overshoot` is how far
// (1.25 for the usual, about a tenth of the distance); ease_back_in and the like are these with the default.

struct EaseBackIn
{
	float overshoot = 1.25f;

	float operator()( float t ) const;
};

struct EaseBackOut
{
	float overshoot = 1.25f;

	float operator()( float t ) const;
};

struct EaseBackInOut
{
	float overshoot = 1.25f;

	float operator()( float t ) const;
};

inline constexpr EaseBackIn ease_back_in{};
inline constexpr EaseBackOut ease_back_out{};
inline constexpr EaseBackInOut ease_back_in_out{};
