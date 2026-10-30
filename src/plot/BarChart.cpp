#include "plot/BarChart.h"

#include <visuals/CreateText.h>
#include <visuals/Line.h>
#include <visuals/Placement.h>

#include <algorithm>
#include <cmath>

BarChart::BarChart( const BarChartOptions &options ) :
	_options( options )
{
}

Rectangle *BarChart::bar( size_t series, size_t category ) const
{
	if ( series >= _bars.size() || category >= _bars[series].size() )
	{
		return nullptr;
	}
	return _bars[series][category];
}

namespace
{

// The most the bars of a category reach up, and down (stacked: the parts added; side by side: the largest)
struct Reach
{
	double up = 0.0;
	double down = 0.0;
};

Reach reach_of( const BarChartOptions &options, size_t category )
{
	Reach reach;
	for ( const BarSeries &series: options.series )
	{
		if ( category >= series.values.size() )
		{
			continue;
		}
		const double v = series.values[category];
		if ( options.stacked )
		{
			( v >= 0.0 ? reach.up : reach.down ) += v;
		}
		else
		{
			reach.up = std::max( reach.up, v );
			reach.down = std::min( reach.down, v );
		}
	}
	return reach;
}

}

std::unique_ptr<BarChart> make_bar_chart( Theme &theme, const BarChartOptions &options )
{
	auto chart = std::make_unique<BarChart>( options );
	const size_t categories = std::max( options.categories.size(), [&]() {
		size_t longest = 0;
		for ( const BarSeries &s: options.series )
			longest = std::max( longest, s.values.size() );
		return longest;
	}() );
	const size_t series_count = options.series.size();

	// The values shown: from 0 (or lower, for negative bars) to the highest, rounded out to the steps
	const float along_values = options.horizontal ? options.width : options.height;
	Range range;
	if ( options.values )
	{
		range = *options.values;
	}
	else
	{
		double up = 0.0;
		double down = 0.0;
		for ( size_t c = 0; c < categories; ++c )
		{
			const Reach reach = reach_of( options, c );
			up = std::max( up, reach.up );
			down = std::min( down, reach.down );
		}
		if ( up == down )
		{
			up = 1.0;
		}
		const double step = options.value_step > 0.0 ? options.value_step : nice_step( up - down, static_cast<double>( along_values ) );
		range = { std::floor( down / step + 1e-9 ) * step, std::ceil( up / step - 1e-9 ) * step };
	}
	const double step = options.value_step > 0.0 ? options.value_step : nice_step( range.span(), static_cast<double>( along_values ) );
	chart->_range = range;

	// The axes: the categories on one, in slots one unit wide (a name in the middle of each), the values on the other
	AxesOptions axes_options;
	axes_options.placement = AxisPlacement::EDGE;
	axes_options.arrows = false;
	axes_options.x_label = "";
	axes_options.y_label = "";
	axes_options.width = options.width;
	axes_options.height = options.height;
	axes_options.color = options.color;
	axes_options.number_style = options.label_style;
	axes_options.grid = options.grid;
	std::vector<TickLabel> names;
	for ( size_t c = 0; c < categories; ++c )
	{
		// The first category at the top, when they run down the side
		const double at = options.horizontal ? static_cast<double>( categories - c ) - 0.5 : static_cast<double>( c ) + 0.5;
		names.push_back( { at, c < options.categories.size() ? options.categories[c] : std::string() } );
	}
	if ( options.horizontal )
	{
		axes_options.x = range;
		axes_options.x_step = step;
		axes_options.y = { 0.0, static_cast<double>( categories ) };
		axes_options.y_tick_labels = names;
		axes_options.grid_y = false; // (lines across the bars' rows: none)
	}
	else
	{
		axes_options.y = range;
		axes_options.y_step = step;
		axes_options.x = { 0.0, static_cast<double>( categories ) };
		axes_options.x_tick_labels = names;
		axes_options.grid_x = false;
	}
	chart->_axes = &chart->add( make_axes( theme, axes_options ) );
	Axes &axes = *chart->_axes;

	// A line at zero where there are bars that go below it, to show where they start
	if ( range.min < 0.0 && range.max > 0.0 )
	{
		auto zero = std::make_unique<Line>();
		zero->start = options.horizontal ? axes.at( 0.0, 0.0 ) : axes.at( 0.0, 0.0 );
		zero->end = options.horizontal ? axes.at( 0.0, static_cast<double>( categories ) ) : axes.at( static_cast<double>( categories ), 0.0 );
		zero->stroke_thickness = axes_options.thickness;
		zero->stroke_color = options.color;
		axes.add_plot( std::move( zero ) );
	}

	for ( const BarSeries &series: options.series )
	{
		chart->_colors.push_back( series.color ? *series.color : axes.next_series_color() );
	}
	const std::vector<Color> accents = theme.palette().series();

	// Each bar: a rectangle from `from` to `to` on the value axis, across `low` to `high` of its category's slot
	auto add_bar = [&]( size_t s, size_t c, double from, double to, double low, double high ) -> Rectangle &
	{
		// (a category on x and a value on y, or the other way round)
		const Point a = options.horizontal ? axes.at( from, low ) : axes.at( low, from );
		const Point b = options.horizontal ? axes.at( to, high ) : axes.at( high, to );
		auto rectangle = std::make_unique<Rectangle>();
		rectangle->half_width = std::abs( b.x() - a.x() ) / 2.F;
		rectangle->half_height = std::abs( b.y() - a.y() ) / 2.F;
		rectangle->position = { ( a.x() + b.x() ) / 2.F, ( a.y() + b.y() ) / 2.F };
		Color color = chart->_colors[s];
		if ( options.color_by_category && series_count == 1 && !accents.empty() )
		{
			color = accents[c % accents.size()];
		}
		rectangle->color = color;
		return axes.add_plot( std::move( rectangle ) );
	};

	// The number on a bar: at its end (outside), or in the middle of the part (stacked)
	auto add_label = [&]( double value, Point where, bool inside )
	{
		auto text = make_text( theme, tick_text( value, step ), options.label_style );
		text->color = options.color;
		const AABB box = box_of( *text );
		const Point middle = { ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F };
		Point target = where;
		if ( !inside )
		{
			// Beyond the end of the bar, away from zero
			const float gap = 0.12F;
			if ( options.horizontal )
				target = where + Point{ ( value >= 0.0 ? 1.F : -1.F ) * ( gap + ( box.max.x() - box.min.x() ) / 2.F ), 0.F };
			else
				target = where + Point{ 0.F, ( value >= 0.0 ? 1.F : -1.F ) * ( gap + ( box.max.y() - box.min.y() ) / 2.F ) };
		}
		text->position = text->position + ( target - middle );
		axes.add_plot( std::move( text ) );
	};

	chart->_bars.assign( series_count, std::vector<Rectangle *>( categories, nullptr ) );
	for ( size_t c = 0; c < categories; ++c )
	{
		const double slot = static_cast<double>( c ) + 0.5;
		const double bar_room = options.horizontal ? static_cast<double>( categories - c ) - 0.5 : slot;
		double up = 0.0;
		double down = 0.0;
		const double group = options.bar_fraction;
		for ( size_t s = 0; s < series_count; ++s )
		{
			const BarSeries &series = options.series[s];
			if ( c >= series.values.size() )
			{
				continue;
			}
			const double v = series.values[c];

			double from = 0.0;
			double to = v;
			double low = bar_room - group / 2.0;
			double high = bar_room + group / 2.0;
			if ( options.stacked )
			{
				double &edge = v >= 0.0 ? up : down;
				from = edge;
				to = edge + v;
				edge = to;
			}
			else
			{
				// The bars of the group side by side, a gap between
				const double width = group / ( static_cast<double>( series_count ) + static_cast<double>( series_count - 1 ) * options.bar_gap );
				low = bar_room - group / 2.0 + static_cast<double>( s ) * width * ( 1.0 + options.bar_gap );
				high = low + width;
			}
			// (values off the top of the range are cut by the plot's edge: keep them in it)
			Rectangle &bar = add_bar( s, c, std::clamp( from, range.min, range.max ), std::clamp( to, range.min, range.max ), low, high );
			chart->_bars[s][c] = &bar;

			if ( options.value_labels )
			{
				const Point end = options.horizontal ? axes.at( to, ( low + high ) / 2.0 ) : axes.at( ( low + high ) / 2.0, to );
				if ( options.stacked )
				{
					const Point middle = options.horizontal ? axes.at( ( from + to ) / 2.0, ( low + high ) / 2.0 ) : axes.at( ( low + high ) / 2.0, ( from + to ) / 2.0 );
					add_label( v, middle, true );
				}
				else
				{
					add_label( v, end, false );
				}
			}
		}
	}
	return chart;
}
