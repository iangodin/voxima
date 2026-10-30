
#pragma once

#include "visuals/Visual.h"
#include "visuals/VisualVisitor.h"
#include "visuals/Point.h"
#include <vector>
#include <variant>

struct MoveTo { Point to; };
struct LineTo { Point to; };
struct QuadTo { Point to; Point ctrl1; };
struct CubicTo { Point to; Point ctrl1; Point ctrl2; };
struct Close { };

using PathCommand = std::variant<MoveTo, LineTo, QuadTo, CubicTo, Close>;

// How the stroke turns a corner. A mitre (the SVG default) is a point, but only up to a limit: a corner
// sharper than about 29 degrees is drawn bevelled instead, so a corner that sharpens while it moves switches
// suddenly. A bevel never switches.
enum class LineJoin
{
	MITER,
	BEVEL,
	ROUND
};

struct Path : public Visual
{
	std::vector<PathCommand> commands;
	LineJoin join = LineJoin::MITER;

	void accept( VisualVisitor &visitor ) const override
	{
		visitor.visit( *this );
	}

	Path &operator*=( float scale ) override
	{
		for ( auto &command: commands )
		{
			std::visit( [scale](auto&& arg) {
				using T = std::decay_t<decltype(arg)>;
				if constexpr (std::is_same_v<T, MoveTo>) arg.to *= scale;
				else if constexpr (std::is_same_v<T, LineTo>) arg.to *= scale;
				else if constexpr (std::is_same_v<T, QuadTo>) { arg.to *= scale; arg.ctrl1 *= scale; }
				else if constexpr (std::is_same_v<T, CubicTo>) { arg.to *= scale; arg.ctrl1 *= scale; arg.ctrl2 *= scale; }
			}, command);
		}
		scale_common( scale );
		return *this;
	}
};
