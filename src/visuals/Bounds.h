#pragma once

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

#include <algorithm>
#include <limits>
#include <variant>

struct AABB
{
	Point min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	Point max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

	void add( Point p )
	{
		min = { std::min( min.x(), p.x() ), std::min( min.y(), p.y() ) };
		max = { std::max( max.x(), p.x() ), std::max( max.y(), p.y() ) };
	}
};

class BoundingBoxVisitor : public VisualVisitor
{
public:
	AABB bounds;

	void visit( const Circle &circle ) override
	{
		bounds.add( { circle.position.x() - circle.radius, circle.position.y() - circle.radius } );
		bounds.add( { circle.position.x() + circle.radius, circle.position.y() + circle.radius } );
	}

	void visit( const Rectangle &rectangle ) override
	{
		bounds.add( { rectangle.position.x() - rectangle.half_width,
		              rectangle.position.y() - rectangle.half_height } );
		bounds.add( { rectangle.position.x() + rectangle.half_width,
		              rectangle.position.y() + rectangle.half_height } );
	}

	void visit( const RoundRectangle &rectangle ) override
	{
		bounds.add( { rectangle.position.x() - rectangle.half_width,
		              rectangle.position.y() - rectangle.half_height } );
		bounds.add( { rectangle.position.x() + rectangle.half_width,
		              rectangle.position.y() + rectangle.half_height } );
	}

	void visit( const Ellipse &ellipse ) override
	{
		bounds.add( { ellipse.position.x() - ellipse.half_width,
		              ellipse.position.y() - ellipse.half_height } );
		bounds.add( { ellipse.position.x() + ellipse.half_width,
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
						bounds.add( { path.position.x() + arg.to.x(), path.position.y() + arg.to.y() } );
					}
					if constexpr ( std::is_same_v<T, QuadTo> || std::is_same_v<T, CubicTo> )
					{
						bounds.add( { path.position.x() + arg.ctrl1.x(), path.position.y() + arg.ctrl1.y() } );
					}
					if constexpr ( std::is_same_v<T, CubicTo> )
					{
						bounds.add( { path.position.x() + arg.ctrl2.x(), path.position.y() + arg.ctrl2.y() } );
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
						bounds.add( { ( glyph.position.x() + arg.to.x() ) * glyph.scale_x,
						              ( glyph.position.y() + arg.to.y() ) * glyph.scale_y } );
					}
					if constexpr ( std::is_same_v<T, QuadTo> || std::is_same_v<T, CubicTo> )
					{
						bounds.add( { ( glyph.position.x() + arg.ctrl1.x() ) * glyph.scale_x,
						              ( glyph.position.y() + arg.ctrl1.y() ) * glyph.scale_y } );
					}
					if constexpr ( std::is_same_v<T, CubicTo> )
					{
						bounds.add( { ( glyph.position.x() + arg.ctrl2.x() ) * glyph.scale_x,
						              ( glyph.position.y() + arg.ctrl2.y() ) * glyph.scale_y } );
					}
				},
				cmd );
		}
	}

	void visit( const Line &line ) override
	{
		bounds.add( line.start );
		bounds.add( line.end );
	}

	void visit( const ArrowTip &arrow ) override
	{
		bounds.add( arrow.start );
		bounds.add( arrow.end );
	}

	void visit( const Group &group ) override
	{
		for ( const auto &visual: group )
		{
			visual->accept( *this );
		}
	}

	void visit( const Image &image ) override
	{
		bounds.add( { image.position.x() - image.half_width, image.position.y() - image.half_height } );
		bounds.add( { image.position.x() + image.half_width, image.position.y() + image.half_height } );
	}
};
