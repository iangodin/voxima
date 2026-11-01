#include "Graph.h"

#include <core/Tween.h>

#include <algorithm>
#include <cmath>

namespace
{

struct Sample
{
	double x;
	double y;
};

// Where the line from a to b crosses the level y = level
Sample crossing( const Sample &a, const Sample &b, double level )
{
	const double t = ( level - a.y ) / ( b.y - a.y );
	return { a.x + ( b.x - a.x ) * t, level };
}

Point lerp( Point a, Point b, float t )
{
	return a + ( b - a ) * t;
}

}

std::unique_ptr<FunctionGraph> plot( const Axes &axes, const std::function<double( double )> &f, const GraphOptions &options )
{
	const AxesOptions &ax = axes.options();
	const Range range = options.x.value_or( ax.x );
	const size_t count = std::max<size_t>( options.samples, 2 );

	auto inside = [&]( const Sample &s ) { return std::isfinite( s.y ) && s.y >= ax.y.min && s.y <= ax.y.max; };
	// A jump across half the plot between neighbouring samples is a break (an asymptote, as tan's), not a line
	auto jump = [&]( const Sample &a, const Sample &b ) { return std::abs( b.y - a.y ) > 0.5 * ax.y.span(); };
	// The edge a sample outside the plot is beyond
	auto edge = [&]( const Sample &s ) { return s.y > ax.y.max ? ax.y.max : ax.y.min; };

	std::vector<std::vector<Sample>> pieces( 1 );
	auto end_piece = [&]()
	{
		if ( !pieces.back().empty() )
		{
			pieces.emplace_back();
		}
	};

	Sample previous{ 0.0, 0.0 };
	for ( size_t i = 0; i < count; ++i )
	{
		const double x = range.min + range.span() * static_cast<double>( i ) / static_cast<double>( count - 1 );
		const Sample s{ x, f( x ) };
		if ( i > 0 )
		{
			const bool was_in = inside( previous );
			const bool is_in = inside( s );
			const bool both_numbers = std::isfinite( previous.y ) && std::isfinite( s.y );
			if ( was_in && is_in && jump( previous, s ) )
			{
				end_piece();
			}
			else if ( was_in && !is_in )
			{
				// Leaving the plot: up to its edge, then a break
				if ( both_numbers )
				{
					pieces.back().push_back( crossing( previous, s, edge( s ) ) );
				}
				end_piece();
			}
			else if ( !was_in && is_in && both_numbers && !jump( previous, s ) )
			{
				// Coming in: from its edge
				pieces.back().push_back( crossing( previous, s, edge( previous ) ) );
			}
		}
		if ( inside( s ) )
		{
			if ( pieces.back().empty() || pieces.back().back().x != s.x )
			{
				pieces.back().push_back( s );
			}
		}
		previous = s;
	}

	auto graph = std::make_unique<FunctionGraph>();
	float distance = 0.F;
	for ( const auto &piece: pieces )
	{
		if ( piece.size() < 2 )
		{
			continue;
		}
		std::vector<Point> points;
		std::vector<float> distances;
		for ( const Sample &s: piece )
		{
			const Point p = axes.at( s.x, s.y );
			if ( !points.empty() )
			{
				distance += std::sqrt( ( p - points.back() ).lengthSquared() );
			}
			points.push_back( p );
			distances.push_back( distance );
		}
		graph->_pieces.push_back( std::move( points ) );
		graph->_distance.push_back( std::move( distances ) );
	}
	graph->_length = distance;

	graph->color = { 1.F, 1.F, 1.F, 0.F }; // a line, not a filled shape
	graph->stroke_color = options.color ? *options.color : axes.next_series_color(); // (only takes the next series color if it is needed)
	graph->stroke_thickness = options.thickness;
	graph->stroke_dash = options.dash;
	graph->_name = options.name;
	graph->join = LineJoin::ROUND;
	graph->set_drawn( 1.F );
	return graph;
}

void FunctionGraph::set_drawn( float fraction )
{
	_drawn = std::clamp( fraction, 0.F, 1.F );
	commands.clear();
	const float target = _drawn * _length;

	// Each piece as cubics through its points (Catmull-Rom: each passes through two points, its tangents set by the
	// points either side), cut off at the target length
	for ( size_t k = 0; k < _pieces.size(); ++k )
	{
		const auto &p = _pieces[k];
		const auto &d = _distance[k];
		if ( d.front() >= target )
		{
			break;
		}
		commands.push_back( MoveTo{ p.front() } );
		const size_t n = p.size();
		for ( size_t i = 0; i + 1 < n; ++i )
		{
			const Point before = p[i == 0 ? 0 : i - 1];
			const Point after = p[std::min( i + 2, n - 1 )];
			Point p0 = p[i];
			Point c1 = p[i] + ( p[i + 1] - before ) / 6.F;
			Point c2 = p[i + 1] - ( after - p[i] ) / 6.F;
			Point p3 = p[i + 1];

			if ( d[i + 1] > target )
			{
				// Only part of this one: the curve up to u (de Casteljau)
				const float u = d[i + 1] > d[i] ? ( target - d[i] ) / ( d[i + 1] - d[i] ) : 0.F;
				const Point a = lerp( p0, c1, u );
				const Point b = lerp( c1, c2, u );
				const Point c = lerp( c2, p3, u );
				const Point ab = lerp( a, b, u );
				const Point bc = lerp( b, c, u );
				commands.push_back( CubicTo{ lerp( ab, bc, u ), a, ab } );
				return;
			}
			commands.push_back( CubicTo{ p3, c1, c2 } );
		}
	}
}

Scene draw( const Stage &stage, FunctionGraph &graph, std::chrono::duration<double> duration )
{
	graph.set_drawn( 0.F );
	co_await tween( stage, duration, []( float t ) { return t; }, [&graph]( float t ) { graph.set_drawn( t ); } );
}
