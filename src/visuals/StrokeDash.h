#pragma once

#include <vector>

// How the ends of the dashes are drawn
enum class LineCap
{
	BUTT,
	ROUND,
	SQUARE
};

// A stroke cut into dashes: `pattern` is lengths on and off in turn, and starts over (repeated once more if it has an odd
// number of them, so { 0.2 } is 0.2 on, 0.2 off), from `offset` along it. Tween the offset for dashes that march along the
// line. No pattern is a whole line. They are in the visual's own units, so they scale with it, as its stroke does.
//
//     line.stroke_dash = StrokeDash::dashed( 0.2F, 0.1F );
//     circle.stroke_dash = StrokeDash::dotted( 0.15F );
//
// A pattern too small to see (under half a pixel) is drawn as a whole line. Each piece of a path (each MoveTo) starts it again.
struct StrokeDash
{
	std::vector<float> pattern = {};
	float offset = 0.F;
	LineCap cap = LineCap::BUTT;

	// Dashes `on` long with gaps `off` long
	static StrokeDash dashed( float on, float off )
	{
		return { { on, off } };
	}

	// Round dots, as wide as the stroke, `spacing` apart
	static StrokeDash dotted( float spacing )
	{
		return { { 0.F, spacing }, 0.F, LineCap::ROUND };
	}

	[[nodiscard]] bool solid() const
	{
		return pattern.empty();
	}

	bool operator==( const StrokeDash & ) const = default;
};
