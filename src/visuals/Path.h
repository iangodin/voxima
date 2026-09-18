
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

struct Path : public Visual
{
	std::vector<PathCommand> commands;

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
