#include "plot/Scatter.h"

#include <visuals/Circle.h>
#include <visuals/Path.h>
#include <visuals/Rectangle.h>

#include <cmath>

namespace
{

// A filled marker with the given corners, around its origin
std::unique_ptr<Path> polygon( std::initializer_list<Point> corners, const Color &color )
{
	auto path = std::make_unique<Path>();
	path->color = color;
	path->join = LineJoin::ROUND;
	path->commands.reserve( corners.size() + 1 );
	bool first = true;
	for ( const Point &corner: corners )
	{
		if ( first )
		{
			path->commands.push_back( MoveTo{ corner } );
			first = false;
		}
		else
		{
			path->commands.push_back( LineTo{ corner } );
		}
	}
	path->commands.push_back( Close{} );
	return path;
}

// Two crossing strokes (× or +), as one path so where they cross they are one stroke
std::unique_ptr<Path> strokes( Point a, Point b, Point c, Point d, const Color &color, float thickness )
{
	auto path = std::make_unique<Path>();
	path->color = { 1.F, 1.F, 1.F, 0.F };
	path->stroke_color = color;
	path->stroke_thickness = thickness;
	path->commands = { MoveTo{ a }, LineTo{ b }, MoveTo{ c }, LineTo{ d } };
	return path;
}

std::unique_ptr<Visual> make_marker( MarkerShape kind, float size, const Color &color, float thickness )
{
	switch ( kind )
	{
		case MarkerShape::SQUARE:
		{
			auto square = std::make_unique<Rectangle>();
			square->half_width = square->half_height = size;
			square->color = color;
			return square;
		}
		case MarkerShape::DIAMOND:
			return polygon( { { 0.F, size * 1.3F }, { size * 1.3F, 0.F }, { 0.F, -size * 1.3F }, { -size * 1.3F, 0.F } }, color );
		case MarkerShape::TRIANGLE:
			return polygon( { { 0.F, size * 1.25F }, { size * 1.15F, -size * 0.85F }, { -size * 1.15F, -size * 0.85F } }, color );
		case MarkerShape::CROSS:
			return strokes( { -size, -size }, { size, size }, { -size, size }, { size, -size }, color, thickness * 1.5F );
		case MarkerShape::PLUS:
			return strokes( { -size, 0.F }, { size, 0.F }, { 0.F, -size }, { 0.F, size }, color, thickness * 1.5F );
		case MarkerShape::CIRCLE:
			break;
	}
	auto circle = std::make_unique<Circle>();
	circle->radius = size;
	circle->color = color;
	return circle;
}

// A run of points as a smooth curve: cubics through them, tangents set by the points either side (Catmull-Rom)
void add_smooth( std::vector<PathCommand> &commands, const std::vector<Point> &p )
{
	commands.push_back( MoveTo{ p.front() } );
	const size_t n = p.size();
	for ( size_t i = 0; i + 1 < n; ++i )
	{
		const Point before = p[i == 0 ? 0 : i - 1];
		const Point after = p[std::min( i + 2, n - 1 )];
		commands.push_back( CubicTo{ p[i + 1], p[i] + ( p[i + 1] - before ) / 6.F, p[i + 1] - ( after - p[i] ) / 6.F } );
	}
}

}

Scatter::Scatter( const ScatterOptions &options, Color color ) :
	_options( options ),
	_color( color )
{
}

size_t Scatter::count() const
{
	return _markers ? _markers->size() : 0;
}

Visual &Scatter::marker( size_t index ) const
{
	return **( _markers->begin() + static_cast<std::ptrdiff_t>( index ) );
}

std::unique_ptr<Scatter> scatter( const Axes &axes, const std::vector<std::pair<double, double>> &points, const ScatterOptions &options )
{
	const AxesOptions &ax = axes.options();
	const Color color = options.color ? *options.color : axes.next_series_color();
	auto result = std::make_unique<Scatter>( options, color );

	// The points in the plot, as runs (a point outside it breaks the line)
	std::vector<std::vector<Point>> runs( 1 );
	auto inside = [&]( double x, double y )
	{
		return std::isfinite( x ) && std::isfinite( y ) && x >= ax.x.min && x <= ax.x.max && y >= ax.y.min && y <= ax.y.max;
	};
	for ( const auto &[x, y]: points )
	{
		if ( inside( x, y ) )
		{
			runs.back().push_back( axes.at( x, y ) );
		}
		else if ( !runs.back().empty() )
		{
			runs.emplace_back();
		}
	}

	// The line first, behind the markers
	if ( options.line != JoinStyle::NONE )
	{
		auto line = std::make_unique<Path>();
		line->color = { 1.F, 1.F, 1.F, 0.F };
		line->stroke_color = color;
		line->stroke_thickness = options.thickness;
		line->join = LineJoin::ROUND;
		for ( const auto &run: runs )
		{
			if ( run.size() < 2 )
			{
				continue;
			}
			if ( options.line == JoinStyle::SMOOTH )
			{
				add_smooth( line->commands, run );
			}
			else
			{
				line->commands.push_back( MoveTo{ run.front() } );
				for ( size_t i = 1; i < run.size(); ++i )
				{
					line->commands.push_back( LineTo{ run[i] } );
				}
			}
		}
		result->_line = &result->add( std::move( line ) );
	}

	result->_markers = &result->add( std::make_unique<Group>() );
	for ( const auto &[x, y]: points )
	{
		if ( inside( x, y ) )
		{
			auto marker = make_marker( options.marker, options.size, color, options.thickness );
			marker->position = axes.at( x, y );
			result->_markers->add( std::move( marker ) );
		}
		else
		{
			result->_markers->add( std::make_unique<Group>() ); // (nothing to draw, but the place is kept)
		}
	}
	return result;
}
