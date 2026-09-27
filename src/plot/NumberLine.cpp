#include "plot/NumberLine.h"

#include <visuals/ArrowTip.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Line.h>
#include <visuals/Placement.h>

#include <algorithm>
#include <cmath>

namespace
{

Line &add_line( Group &group, Point from, Point to, float thickness, const Color &color )
{
	auto line = std::make_unique<Line>();
	line->start = from;
	line->end = to;
	line->stroke_thickness = thickness;
	line->stroke_color = color;
	return group.add( std::move( line ) );
}

}

NumberLine::NumberLine( const NumberLineOptions &options ) :
	_options( options )
{
}

Point NumberLine::at( double value ) const
{
	const double t = ( value - _options.range.min ) / _options.range.span();
	return { static_cast<float>( ( t - 0.5 ) * static_cast<double>( _options.length ) ), 0.F };
}

Group &NumberLine::add_mark( Theme &theme, double value, const std::string &label, const Color &color, const FontStyle &style )
{
	auto mark = std::make_unique<Group>();
	auto dot = std::make_unique<Circle>();
	dot->radius = 0.13F;
	dot->color = color;
	dot->position = at( value );
	mark->add( std::move( dot ) );
	if ( !label.empty() )
	{
		auto &text = mark->add( make_text( theme, label, style ) );
		text.color = color;
		move_to( text, at( value ) + Point{ 0.F, 0.3F }, Anchor::BOTTOM );
	}
	return add( std::move( mark ) );
}

Group &NumberLine::add_interval( Theme &theme, double from, double to, const Color &color, const IntervalOptions &options )
{
	auto interval = std::make_unique<Group>();
	Line &bar = add_line( *interval, at( from ), at( to ), options.thickness, color );
	bar.stroke_color = color;

	auto end = [&]( double value, bool closed )
	{
		auto dot = std::make_unique<Circle>();
		dot->radius = 0.16F;
		dot->position = at( value );
		dot->stroke_thickness = 0.05F;
		dot->stroke_color = color;
		// A hollow end shows the line under it as empty: the dot is the background's color
		dot->color = closed ? color : theme.palette().background_fill();
		interval->add( std::move( dot ) );
	};
	end( from, options.closed_from );
	end( to, options.closed_to );
	return add( std::move( interval ) );
}

std::unique_ptr<NumberLine> make_number_line( Theme &theme, const NumberLineOptions &options )
{
	auto line = std::make_unique<NumberLine>( options );
	const float half = options.length / 2.F;
	const double step = options.step > 0.0 ? options.step : nice_step( options.range.span(), static_cast<double>( options.length ) );

	const float tip = 0.18F;
	add_line( *line, { -half, 0.F }, { half - ( options.arrows ? tip : 0.F ), 0.F }, options.thickness, options.color );
	if ( options.arrows )
	{
		auto arrow = std::make_unique<ArrowTip>();
		arrow->start = { half - 2.F * tip, 0.F };
		arrow->end = { half, 0.F };
		arrow->size = tip;
		arrow->color = options.color;
		line->add( std::move( arrow ) );
	}

	for ( double value: tick_values( options.range, step ) )
	{
		const Point p = line->at( value );
		if ( options.arrows && half - p.x() < tip * 3.F )
		{
			continue; // (not under the arrow tip)
		}
		add_line( *line, p + Point{ 0.F, options.tick_length }, p - Point{ 0.F, options.tick_length }, options.thickness, options.color );
		if ( options.numbers )
		{
			auto &number = line->add( make_text( theme, tick_text( value, step ), options.number_style ) );
			number.color = options.color;
			move_to( number, p - Point{ 0.F, options.tick_length + 0.12F }, Anchor::TOP );
		}
	}
	return line;
}
