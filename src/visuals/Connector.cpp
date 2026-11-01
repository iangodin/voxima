#include "visuals/Connector.h"

#include "core/Stage.h"
#include "visuals/Placement.h"

#include <algorithm>
#include <cmath>

namespace
{

// Where a ray from the box's center, toward `toward`, leaves the box
Point leave( const AABB &box, Point toward )
{
	const Point center = { ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F };
	const float dx = toward.x() - center.x();
	const float dy = toward.y() - center.y();
	const float half_width = ( box.max.x() - box.min.x() ) / 2.F;
	const float half_height = ( box.max.y() - box.min.y() ) / 2.F;
	if ( dx == 0.F && dy == 0.F )
	{
		return center;
	}
	// How far along the ray (in units of the vector) it takes to reach each pair of sides
	const float tx = dx != 0.F ? half_width / std::abs( dx ) : INFINITY;
	const float ty = dy != 0.F ? half_height / std::abs( dy ) : INFINITY;
	const float t = std::min( tx, ty );
	return center + Point{ dx, dy } * t;
}

}

Connector::Connector( const Visual &from, const Visual &to, const ConnectorOptions &options ) :
	_from( &from ),
	_to( &to ),
	_options( options )
{
	_line = &add( std::make_unique<Line>() );
	_line->stroke_color = options.color;
	_line->stroke_thickness = options.thickness;
	_line->stroke_dash = options.dash;

	auto tip = [&]()
	{
		auto arrow = std::make_unique<ArrowTip>();
		arrow->color = options.color;
		arrow->size = options.arrow_size;
		return arrow;
	};
	_end_tip = &add( tip() );
	_start_tip = &add( tip() );
	update();
}

void Connector::update()
{
	const AABB a = box_of( *_from );
	const AABB b = box_of( *_to );
	const Point a_center = { ( a.min.x() + a.max.x() ) / 2.F, ( a.min.y() + a.max.y() ) / 2.F };
	const Point b_center = { ( b.min.x() + b.max.x() ) / 2.F, ( b.min.y() + b.max.y() ) / 2.F };

	Point start = leave( a, b_center );
	Point end = leave( b, a_center );
	const Point along = end - start;
	const float length = std::sqrt( along.lengthSquared() );
	const Point direction = length > 1e-6F ? along / length : Point{ 1.F, 0.F };

	start = start + direction * _options.gap;
	end = end - direction * _options.gap;

	const float tip_length = _options.arrow_size * 2.F;
	Point line_start = start;
	Point line_end = end;
	// Never a tip longer than the connector
	const bool room = std::sqrt( ( end - start ).lengthSquared() ) > tip_length * ( _options.arrow_at_start ? 2.F : 1.F );
	const bool at_end = _options.arrow && room;
	const bool at_start = _options.arrow_at_start && room;
	if ( at_end )
	{
		line_end = end - direction * tip_length;
	}
	if ( at_start )
	{
		line_start = start + direction * tip_length;
	}
	_line->start = line_start;
	_line->end = line_end;

	// A hidden tip is one of no size
	_end_tip->start = start;
	_end_tip->end = end;
	_end_tip->size = at_end ? _options.arrow_size : 0.F;
	_start_tip->start = end;
	_start_tip->end = start;
	_start_tip->size = at_start ? _options.arrow_size : 0.F;
}

std::unique_ptr<Connector> make_connector( const Visual &from, const Visual &to, const ConnectorOptions &options )
{
	return std::make_unique<Connector>( from, to, options );
}

Updaters::Handle follow( Stage &stage, Connector &connector )
{
	return stage.updaters.add( [&connector] { connector.update(); } );
}
