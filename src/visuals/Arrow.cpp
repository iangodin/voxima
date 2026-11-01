#include "visuals/Arrow.h"

#include "core/Stage.h"
#include "core/Tween.h"
#include "visuals/Trim.h"

#include <algorithm>

namespace
{
// A head's share of the time it takes to draw the arrow
constexpr float TIP_SHARE = 0.2F;

// The first `fraction` of a contour, as a contour
Contour first_part( const Contour &contour, float fraction )
{
	Path path;
	path.commands = trimmed_commands( contour, fraction );
	return contour_of( path );
}

// How far along a contour it is `distance` in a straight line from its end, which is where a head of that length starts. Lengths
// along a curve are only worked out roughly (see Trim.h), and going by those would leave a gap between the line and the head.
float short_of_end( const Contour &contour, float distance )
{
	const Point end = contour.segs.back().to;
	float low = std::max( 0.F, 1.F - 2.F * distance / contour_length( contour ) );
	float high = 1.F;
	for ( int i = 0; i < 24; ++i )
	{
		const float middle = ( low + high ) / 2.F;
		if ( ( point_along( contour, middle ) - end ).lengthSquared() > distance * distance )
		{
			low = middle;
		}
		else
		{
			high = middle;
		}
	}
	return low;
}

float between( float value, float from, float to )
{
	return to > from ? std::clamp( ( value - from ) / ( to - from ), 0.F, 1.F ) : 1.F;
}
}

Arrow::Arrow( const Contour &contour, const ConnectorOptions &options ) :
	_contour( contour ),
	_options( options )
{
	_contour.closed = false; // it has two ends, even if they are in one place

	auto shaft = std::make_unique<Path>();
	shaft->color = { 1.F, 1.F, 1.F, 0.F };
	shaft->stroke_color = options.color;
	shaft->stroke_thickness = options.thickness;
	shaft->stroke_dash = options.dash;
	shaft->join = LineJoin::ROUND;
	_shaft = &add( std::move( shaft ) );

	auto tip = [&]()
	{
		auto arrow = std::make_unique<ArrowTip>();
		arrow->color = options.color;
		return arrow;
	};
	_end_tip = &add( tip() );
	_start_tip = &add( tip() );
	lay_out();
	set_drawn( 1.F );
}

void Arrow::lay_out()
{
	const float length = contour_length( _contour );
	const float tip_length = _options.arrow_size * 2.F;
	// Never a head longer than the arrow
	const bool room = length > tip_length * ( _options.arrow && _options.arrow_at_start ? 2.F : 1.F );
	_has_end_tip = _options.arrow && room;
	_has_start_tip = _options.arrow_at_start && room;

	_line = _contour;
	if ( _has_end_tip )
	{
		const float along = short_of_end( _line, tip_length );
		_end_base = point_along( _line, along );
		_line = first_part( _line, along );
	}
	if ( _has_start_tip )
	{
		// (the same from the other end, on the line walked the other way)
		const Contour backward = reversed( _line );
		const float along = short_of_end( backward, tip_length );
		_start_base = point_along( backward, along );
		_line = reversed( first_part( backward, along ) );
	}
}

void Arrow::set_drawn( float fraction )
{
	_drawn = std::clamp( fraction, 0.F, 1.F );
	const float line_from = _has_start_tip ? TIP_SHARE : 0.F;
	const float line_to = _has_end_tip ? 1.F - TIP_SHARE : 1.F;
	_shaft->commands = trimmed_commands( _line, between( _drawn, line_from, line_to ) );

	// A head grows out of the end of the line, toward where its point goes. One that is not there has no size.
	const auto place = [&]( ArrowTip &tip, bool there, Point base, Point point, float grown )
	{
		tip.start = base;
		tip.size = there ? _options.arrow_size * grown : 0.F;
		tip.end = there ? base + ( point - base ) * grown : base;
	};
	const Point start = _contour.start;
	const Point end = _contour.segs.empty() ? start : _contour.segs.back().to;
	place( *_end_tip, _has_end_tip, _end_base, end, between( _drawn, line_to, 1.F ) );
	place( *_start_tip, _has_start_tip, _start_base, start, between( _drawn, 0.F, line_from ) );
}

Arrow &Arrow::operator*=( float scale )
{
	Group::operator*=( scale );
	_contour.start *= scale;
	for ( CubicSeg &seg: _contour.segs )
	{
		seg.c1 *= scale;
		seg.c2 *= scale;
		seg.to *= scale;
	}
	_options.arrow_size *= scale;
	lay_out();
	set_drawn( _drawn );
	return *this;
}

std::unique_ptr<Arrow> make_arrow( Point from, Point to, const ConnectorOptions &options )
{
	Contour contour;
	contour.closed = false;
	contour.start = from;
	contour.segs.push_back( { from + ( to - from ) * ( 1.F / 3.F ), from + ( to - from ) * ( 2.F / 3.F ), to } );
	return std::make_unique<Arrow>( contour, options );
}

std::unique_ptr<Arrow> make_arrow( const Path &path, const ConnectorOptions &options )
{
	auto arrow = std::make_unique<Arrow>( contour_of( path ), options );
	arrow->position = path.position;
	arrow->rotation = path.rotation;
	arrow->scale_x = path.scale_x;
	arrow->scale_y = path.scale_y;
	return arrow;
}

Scene draw( const Stage &stage, Arrow &arrow, std::chrono::duration<double> duration )
{
	arrow.set_drawn( 0.F );
	co_await tween( stage, duration, []( float t ) { return t; }, [&arrow]( float t ) { arrow.set_drawn( t ); } );
}
