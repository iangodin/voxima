#include "Axes.h"

#include <core/Theme.h>
#include <visuals/ArrowTip.h>
#include <visuals/Bounds.h>
#include <visuals/CreateText.h>
#include <visuals/Glyph.h>
#include <visuals/Line.h>
#include <visuals/Path.h>

#include <cmath>
#include <map>
#include <cstdio>

double nice_step( double span, double count )
{
	if ( !( span > 0.0 ) || !( count > 0.0 ) )
	{
		return 1.0;
	}
	const double raw = span / count;
	const double magnitude = std::pow( 10.0, std::floor( std::log10( raw ) ) );
	const double f = raw / magnitude;
	const double nice = f < 1.5 ? 1.0 : f < 3.5 ? 2.0 : f < 7.5 ? 5.0 : 10.0;
	return nice * magnitude;
}

std::vector<double> tick_values( const Range &range, double step )
{
	std::vector<double> values;
	if ( !( step > 0.0 ) )
	{
		return values;
	}
	const auto first = static_cast<long long>( std::ceil( range.min / step - 1e-9 ) );
	const auto last = static_cast<long long>( std::floor( range.max / step + 1e-9 ) );
	for ( long long k = first; k <= last; ++k )
	{
		values.push_back( k == 0 ? 0.0 : static_cast<double>( k ) * step );
	}
	return values;
}

std::string tick_text( double value, double step )
{
	const int decimals = step > 0.0 ? std::max( 0, static_cast<int>( -std::floor( std::log10( step ) + 1e-9 ) ) ) : 0;
	char buffer[64];
	std::snprintf( buffer, sizeof( buffer ), "%.*f", decimals, std::abs( value ) );
	std::string text = buffer;
	if ( value < 0.0 && text.find_first_not_of( "0." ) != std::string::npos )
	{
		text = "−" + text; // a minus sign, not a hyphen
	}
	return text;
}

Axes::Axes( const AxesOptions &options ) :
	_options( options )
{
	// The layers, bottom up: the grid, the axes with their ticks, and what is plotted (add_plot); the numbers and
	// names go on top of those
	_grid = &add( std::make_unique<Group>() );
	_lines = &add( std::make_unique<Group>() );
	_plots = &add( std::make_unique<Group>() );
}

Point Axes::at( double x, double y ) const
{
	const double fx = ( x - _options.x.min ) / _options.x.span();
	const double fy = ( y - _options.y.min ) / _options.y.span();
	return { left() + static_cast<float>( fx ) * _options.width, bottom() + static_cast<float>( fy ) * _options.height };
}

double Axes::x_crossing() const
{
	const bool at_zero = _options.placement == AxisPlacement::ORIGIN && _options.x.min <= 0.0 && 0.0 <= _options.x.max;
	return at_zero ? 0.0 : _options.x.min;
}

double Axes::y_crossing() const
{
	const bool at_zero = _options.placement == AxisPlacement::ORIGIN && _options.y.min <= 0.0 && 0.0 <= _options.y.max;
	return at_zero ? 0.0 : _options.y.min;
}

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

// Text laid out, and its inked box (baseline at 0), to be placed by the box
struct Placed
{
	std::unique_ptr<Group> text;
	AABB box;
};

Placed text_of( Theme &theme, const std::string &text, const FontStyle &style, const Color &color )
{
	Placed placed{ make_text( theme, text, style ), {} };
	placed.text->color = color;
	BoundingBoxVisitor ink;
	placed.text->accept( ink );
	placed.box = ink.bounds;
	return placed;
}

// Every glyph below, filled and outlined in one color (a halo, drawn under the text it copies)
void make_halo( Visual &visual, const Color &color, float width )
{
	if ( auto *glyph = dynamic_cast<Glyph *>( &visual ) )
	{
		glyph->color = color;
		glyph->stroke_color = color;
		glyph->stroke_thickness = width;
	}
	else if ( auto *group = dynamic_cast<Group *>( &visual ) )
	{
		group->color = { 1.F, 1.F, 1.F, 1.F };
		for ( const auto &child: *group )
		{
			make_halo( *child, color, width );
		}
	}
}

// A number, where it goes, with its halo under it if the options have one
void add_number( Group &axes, Theme &theme, const AxesOptions &options, const std::string &text, Point where )
{
	if ( options.number_halo )
	{
		Placed halo = text_of( theme, text, options.number_style, options.color );
		make_halo( *halo.text, *options.number_halo, options.halo_width );
		halo.text->position = where;
		axes.add( std::move( halo.text ) );
	}
	Placed number = text_of( theme, text, options.number_style, options.color );
	number.text->position = where;
	axes.add( std::move( number.text ) );
}

}

std::unique_ptr<Axes> make_axes( Theme &theme, const AxesOptions &options )
{
	auto axes = std::make_unique<Axes>( options );
	axes->_series = theme.palette().series();
	const double x_step = options.x_step > 0.0 ? options.x_step : nice_step( options.x.span(), options.width );
	const double y_step = options.y_step > 0.0 ? options.y_step : nice_step( options.y.span(), options.height );
	axes->_x_ticks = tick_values( options.x, x_step );
	axes->_y_ticks = tick_values( options.y, y_step );
	// Ticks of your own take the place of the ones at even steps, with their own texts (kept in step with the values)
	std::map<double, std::string> x_texts;
	std::map<double, std::string> y_texts;
	if ( !options.x_tick_labels.empty() )
	{
		axes->_x_ticks.clear();
		for ( const TickLabel &tick: options.x_tick_labels )
		{
			axes->_x_ticks.push_back( tick.value );
			x_texts[tick.value] = tick.text;
		}
	}
	if ( !options.y_tick_labels.empty() )
	{
		axes->_y_ticks.clear();
		for ( const TickLabel &tick: options.y_tick_labels )
		{
			axes->_y_ticks.push_back( tick.value );
			y_texts[tick.value] = tick.text;
		}
	}

	const double xc = axes->x_crossing(); // where the y axis is
	const double yc = axes->y_crossing(); // where the x axis is
	const float tick = options.tick_length;
	const float gap = 0.1F;
	const float tip = 3.5F * options.thickness;

	// The grid in its layer, under everything else, as one path with one stroke: separate see-through lines would
	// blend twice where they cross, and show a bright spot at every crossing
	if ( options.grid )
	{
		auto grid = std::make_unique<Path>();
		grid->color = { 1.F, 1.F, 1.F, 0.F }; // lines, not a filled shape
		grid->stroke_color = options.grid_color;
		grid->stroke_thickness = options.thickness * 0.6F;
		for ( double x: axes->_x_ticks )
		{
			if ( !options.grid_x )
				break;
			grid->commands.push_back( MoveTo{ axes->at( x, options.y.min ) } );
			grid->commands.push_back( LineTo{ axes->at( x, options.y.max ) } );
		}
		for ( double y: axes->_y_ticks )
		{
			if ( !options.grid_y )
				break;
			grid->commands.push_back( MoveTo{ axes->at( options.x.min, y ) } );
			grid->commands.push_back( LineTo{ axes->at( options.x.max, y ) } );
		}
		axes->_grid->add( std::move( grid ) );
	}

	// The axes, and their arrow tips
	const Point x_from = axes->at( options.x.min, yc );
	const Point x_to = axes->at( options.x.max, yc );
	const Point y_from = axes->at( xc, options.y.min );
	const Point y_to = axes->at( xc, options.y.max );
	// With an arrow tip the line stops at its base (the tip is twice its size long): a line that ran on to the point would
	// stick out past the narrowing head
	auto stop_at_tip = [&]( Point from, Point to )
	{
		if ( !options.arrows )
		{
			return to;
		}
		const Point along = to - from;
		const float length = std::sqrt( along.lengthSquared() );
		return length > 2.F * tip ? to - along * ( 2.F * tip / length ) : to;
	};
	add_line( *axes->_lines, x_from, stop_at_tip( x_from, x_to ), options.thickness, options.color );
	add_line( *axes->_lines, y_from, stop_at_tip( y_from, y_to ), options.thickness, options.color );
	if ( options.arrows )
	{
		for ( auto [from, to]: { std::pair{ x_from, x_to }, std::pair{ y_from, y_to } } )
		{
			auto arrow = std::make_unique<ArrowTip>();
			arrow->start = from;
			arrow->end = to;
			arrow->size = tip;
			arrow->color = options.color;
			axes->_lines->add( std::move( arrow ) );
		}
	}

	// A tick is left out under an arrow tip, and (with the axes crossing in the middle) where the other axis crosses:
	// the origin's two numbers would run together. Along the edges every tick is kept, the corner's too.
	auto ticked = [&]( double value, double crossing, double end, double units_per_stage_unit )
	{
		const bool at_crossing = options.placement == AxisPlacement::ORIGIN && std::abs( value - crossing ) < 1e-9 * std::max( 1.0, std::abs( crossing ) );
		const bool under_tip = options.arrows && ( end - value ) / units_per_stage_unit < tip * 1.5F;
		return !at_crossing && !under_tip;
	};

	for ( double x: axes->_x_ticks )
	{
		if ( !ticked( x, xc, options.x.max, options.x.span() / options.width ) )
		{
			continue;
		}
		const Point p = axes->at( x, yc );
		add_line( *axes->_lines, { p.x(), p.y() - tick / 2.F }, { p.x(), p.y() + tick / 2.F }, options.thickness, options.color );
		if ( options.numbers )
		{
			// Centred under the tick
			const std::string text = x_texts.empty() ? tick_text( x, x_step ) : x_texts[x];
			const AABB box = text_of( theme, text, options.number_style, options.color ).box;
			add_number( *axes, theme, options, text, { p.x() - ( box.min.x() + box.max.x() ) / 2.F, p.y() - tick / 2.F - gap - box.max.y() } );
		}
	}
	for ( double y: axes->_y_ticks )
	{
		if ( !ticked( y, yc, options.y.max, options.y.span() / options.height ) )
		{
			continue;
		}
		const Point p = axes->at( xc, y );
		add_line( *axes->_lines, { p.x() - tick / 2.F, p.y() }, { p.x() + tick / 2.F, p.y() }, options.thickness, options.color );
		if ( options.numbers )
		{
			// To the left of the tick, its middle level with it
			const std::string text = y_texts.empty() ? tick_text( y, y_step ) : y_texts[y];
			const AABB box = text_of( theme, text, options.number_style, options.color ).box;
			add_number( *axes, theme, options, text, { p.x() - tick / 2.F - gap - box.max.x(), p.y() - ( box.min.y() + box.max.y() ) / 2.F } );
		}
	}

	// What the axes are: past the end of the x axis, and above the y axis
	if ( !options.x_label.empty() )
	{
		Placed label = text_of( theme, options.x_label, options.label_style, options.color );
		label.text->position = { x_to.x() + 2.F * gap - label.box.min.x(), x_to.y() - ( label.box.min.y() + label.box.max.y() ) / 2.F };
		axes->add( std::move( label.text ) );
	}
	if ( !options.y_label.empty() )
	{
		Placed label = text_of( theme, options.y_label, options.label_style, options.color );
		label.text->position = { y_to.x() - ( label.box.min.x() + label.box.max.x() ) / 2.F, y_to.y() + 2.F * gap - label.box.min.y() };
		axes->add( std::move( label.text ) );
	}

	return axes;
}
