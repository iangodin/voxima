#pragma once

#include "visuals/AABB.h"
#include "visuals/Point.h"
#include "visuals/VisualVisitor.h"
#include "visuals/Circle.h"
#include "visuals/Rectangle.h"
#include "visuals/RoundRectangle.h"
#include "visuals/Ellipse.h"
#include "visuals/Line.h"
#include "visuals/ArrowTip.h"
#include "visuals/Glyph.h"
#include "visuals/Path.h"
#include "visuals/Group.h"
#include "visuals/Image.h"
#include "visuals/MathAssembly.h"

#include <algorithm>
#include <limits>
#include <variant>

class BoundingBoxVisitor : public VisualVisitor
{
public:
	AABB bounds;

	void visit( const Circle &circle ) override
	{
		add( { circle.position.x() - circle.radius, circle.position.y() - circle.radius } );
		add( { circle.position.x() + circle.radius, circle.position.y() + circle.radius } );
	}

	void visit( const Rectangle &rectangle ) override
	{
		add( { rectangle.position.x() - rectangle.half_width,
		              rectangle.position.y() - rectangle.half_height } );
		add( { rectangle.position.x() + rectangle.half_width,
		              rectangle.position.y() + rectangle.half_height } );
	}

	void visit( const RoundRectangle &rectangle ) override
	{
		add( { rectangle.position.x() - rectangle.half_width,
		              rectangle.position.y() - rectangle.half_height } );
		add( { rectangle.position.x() + rectangle.half_width,
		              rectangle.position.y() + rectangle.half_height } );
	}

	void visit( const Ellipse &ellipse ) override
	{
		add( { ellipse.position.x() - ellipse.half_width,
		              ellipse.position.y() - ellipse.half_height } );
		add( { ellipse.position.x() + ellipse.half_width,
		              ellipse.position.y() + ellipse.half_height } );
	}

	void visit( const Path &path ) override
	{
		for ( const auto &cmd: path.commands )
		{
			std::visit(
				[&]( auto &&arg )
				{
					using T = std::decay_t<decltype( arg )>;
					if constexpr ( std::is_same_v<T, MoveTo> || std::is_same_v<T, LineTo> ||
					               std::is_same_v<T, QuadTo> || std::is_same_v<T, CubicTo> )
					{
						add( { path.position.x() + arg.to.x(), path.position.y() + arg.to.y() } );
					}
					if constexpr ( std::is_same_v<T, QuadTo> || std::is_same_v<T, CubicTo> )
					{
						add( { path.position.x() + arg.ctrl1.x(), path.position.y() + arg.ctrl1.y() } );
					}
					if constexpr ( std::is_same_v<T, CubicTo> )
					{
						add( { path.position.x() + arg.ctrl2.x(), path.position.y() + arg.ctrl2.y() } );
					}
				},
				cmd );
		}
	}

	void visit( const Glyph &glyph ) override
	{
		for ( const auto &cmd: glyph.commands )
		{
			std::visit(
				[&]( auto &&arg )
				{
					using T = std::decay_t<decltype( arg )>;
					if constexpr ( std::is_same_v<T, MoveTo> || std::is_same_v<T, LineTo> ||
					               std::is_same_v<T, QuadTo> || std::is_same_v<T, CubicTo> )
					{
						add( { ( glyph.position.x() + arg.to.x() ) * glyph.scale_x,
						              ( glyph.position.y() + arg.to.y() ) * glyph.scale_y } );
					}
					if constexpr ( std::is_same_v<T, QuadTo> || std::is_same_v<T, CubicTo> )
					{
						add( { ( glyph.position.x() + arg.ctrl1.x() ) * glyph.scale_x,
						              ( glyph.position.y() + arg.ctrl1.y() ) * glyph.scale_y } );
					}
					if constexpr ( std::is_same_v<T, CubicTo> )
					{
						add( { ( glyph.position.x() + arg.ctrl2.x() ) * glyph.scale_x,
						              ( glyph.position.y() + arg.ctrl2.y() ) * glyph.scale_y } );
					}
				},
				cmd );
		}
	}

	void visit( const Line &line ) override
	{
		add( line.start );
		add( line.end );
	}

	void visit( const ArrowTip &arrow ) override
	{
		add( arrow.start );
		add( arrow.end );
	}

	void visit( const Group &group ) override
	{
		Point old_offset = _offset;
		_offset = _offset + group.position;
		for ( const auto &visual: group )
		{
			visual->accept( *this );
		}
		_offset = old_offset;
	}

	void visit( const MathAssembly &assembly ) override
	{
		visit( static_cast<const Group &>( assembly ) );
	}

	void visit( const Image &image ) override
	{
		add( { image.position.x() - image.half_width, image.position.y() - image.half_height } );
		add( { image.position.x() + image.half_width, image.position.y() + image.half_height } );
	}

private:
	void add( Point p ) { bounds.add( _offset + p ); }

	Point _offset = { 0.F, 0.F };
};
