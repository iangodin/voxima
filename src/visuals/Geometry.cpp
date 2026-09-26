#include "visuals/Geometry.h"

#include "visuals/Circle.h"
#include "visuals/Placement.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{

constexpr float PI = std::numbers::pi_v<float>;

Point unit( Point v )
{
	const float length = std::sqrt( v.lengthSquared() );
	return length > 1e-6F ? v / length : Point{ 1.F, 0.F };
}

float heading( Point v )
{
	return std::atan2( v.y(), v.x() );
}

Point on_circle( Point center, float radius, float angle )
{
	return center + Point{ std::cos( angle ), std::sin( angle ) } * radius;
}

// Set a text so that its box's middle is at `middle`
void center_at( Visual &text, Point middle )
{
	move_to( text, middle, Anchor::CENTER );
}

// Put a label so that it stands out from `from` along `direction`: its middle where the box's edge, that direction, is a gap
// past `from`
void place_beyond( Visual &text, Point from, Point direction, float gap )
{
	const AABB box = box_of( text );
	const float reach = std::abs( direction.x() ) * ( box.max.x() - box.min.x() ) / 2.F + std::abs( direction.y() ) * ( box.max.y() - box.min.y() ) / 2.F;
	center_at( text, from + direction * ( gap + reach ) );
}

}

float angle_between( Point vertex, Point a, Point b )
{
	const Point u = unit( a - vertex );
	const Point v = unit( b - vertex );
	const float cosine = std::clamp( u.x() * v.x() + u.y() * v.y(), -1.F, 1.F );
	return std::acos( cosine ) * 180.F / PI;
}

std::unique_ptr<Group> make_polygon( Theme &theme, const std::vector<Point> &points, const PolygonOptions &options )
{
	auto group = std::make_unique<Group>();

	auto outline = std::make_unique<Path>();
	outline->color = options.fill ? *options.fill : Color{ 1.F, 1.F, 1.F, 0.F };
	outline->stroke_color = options.outline;
	outline->stroke_thickness = options.thickness;
	outline->join = LineJoin::ROUND;
	outline->commands.reserve( points.size() + 1 );
	for ( size_t i = 0; i < points.size(); ++i )
	{
		if ( i == 0 )
		{
			outline->commands.push_back( MoveTo{ points[i] } );
		}
		else
		{
			outline->commands.push_back( LineTo{ points[i] } );
		}
	}
	if ( !points.empty() )
	{
		outline->commands.push_back( Close{} );
	}
	group->add( std::move( outline ) );

	Point middle{ 0.F, 0.F };
	for ( const Point &p: points )
	{
		middle = middle + p;
	}
	if ( !points.empty() )
	{
		middle = middle / static_cast<float>( points.size() );
	}

	if ( options.dots )
	{
		for ( const Point &p: points )
		{
			auto dot = std::make_unique<Circle>();
			dot->radius = options.thickness * 2.5F;
			dot->color = options.outline;
			dot->position = p;
			group->add( std::move( dot ) );
		}
	}

	for ( size_t i = 0; i < points.size() && i < options.labels.size(); ++i )
	{
		if ( options.labels[i].empty() )
		{
			continue;
		}
		auto &text = group->add( make_text( theme, options.labels[i], options.label_style ) );
		place_beyond( text, points[i], unit( points[i] - middle ), options.label_gap );
	}
	return group;
}

std::unique_ptr<Group> make_angle( Theme &theme, Point vertex, Point a, Point b, const AngleOptions &options )
{
	auto group = std::make_unique<Group>();
	const float first = heading( a - vertex );
	float sweep = heading( b - vertex ) - first;
	while ( sweep > PI ) sweep -= 2.F * PI;
	while ( sweep < -PI ) sweep += 2.F * PI;

	auto mark = std::make_unique<Path>();
	mark->color = { 1.F, 1.F, 1.F, 0.F };
	mark->stroke_color = options.color;
	mark->stroke_thickness = options.thickness;
	mark->join = LineJoin::MITER;

	const bool right = std::abs( std::abs( sweep ) - PI / 2.F ) < 0.01F;
	if ( right && options.square_for_right )
	{
		mark->commands.reserve( 3 );
		const Point u = unit( a - vertex ) * ( options.radius * 0.7F );
		const Point v = unit( b - vertex ) * ( options.radius * 0.7F );
		mark->commands.push_back( MoveTo{ vertex + u } );
		mark->commands.push_back( LineTo{ vertex + u + v } );
		mark->commands.push_back( LineTo{ vertex + v } );
	}
	else
	{
		// The arc in pieces of at most a quarter turn: a cubic through the ends, its handles a tangent long
		const int pieces = std::max( 1, static_cast<int>( std::ceil( std::abs( sweep ) / ( PI / 2.F ) - 1e-4F ) ) );
		const float step = sweep / static_cast<float>( pieces );
		const float handle = 4.F / 3.F * std::tan( step / 4.F ) * options.radius;
		mark->commands.reserve( static_cast<size_t>( pieces ) + 1 );
		mark->commands.push_back( MoveTo{ on_circle( vertex, options.radius, first ) } );
		for ( int i = 0; i < pieces; ++i )
		{
			const float from = first + step * static_cast<float>( i );
			const float to = from + step;
			const Point tangent_from = Point{ -std::sin( from ), std::cos( from ) } * handle;
			const Point tangent_to = Point{ -std::sin( to ), std::cos( to ) } * handle;
			mark->commands.push_back( CubicTo{ on_circle( vertex, options.radius, to ),
				on_circle( vertex, options.radius, from ) + tangent_from,
				on_circle( vertex, options.radius, to ) - tangent_to } );
		}
	}
	group->add( std::move( mark ) );

	if ( !options.label.empty() )
	{
		auto &text = group->add( make_text( theme, options.label, options.label_style ) );
		const float middle = first + sweep / 2.F;
		place_beyond( text, on_circle( vertex, options.radius, middle ), { std::cos( middle ), std::sin( middle ) }, options.label_gap );
	}
	return group;
}
