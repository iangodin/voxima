#include "plot/Legend.h"

#include <visuals/CreateText.h>
#include <visuals/Line.h>
#include <visuals/Rectangle.h>

#include <algorithm>

namespace
{

constexpr float LINE_HEIGHT = 1.2F; // in ems: what make_text lays a line out at

std::unique_ptr<Visual> make_swatch( const LegendEntry &entry, const LegendOptions &options )
{
	const float s = options.swatch_size;
	auto line = [&]()
	{
		auto stroke = std::make_unique<Line>();
		stroke->start = { -s * 2.F, 0.F };
		stroke->end = { s * 2.F, 0.F };
		stroke->stroke_thickness = 0.05F;
		stroke->stroke_color = entry.color;
		return stroke;
	};
	switch ( entry.swatch )
	{
		case Swatch::LINE:
			return line();
		case Swatch::MARKER:
			return make_marker( entry.marker, s, entry.color );
		case Swatch::LINE_MARKER:
		{
			auto group = std::make_unique<Group>();
			group->add( line() );
			group->add( make_marker( entry.marker, s, entry.color ) );
			return group;
		}
		case Swatch::SQUARE:
			break;
	}
	auto square = std::make_unique<Rectangle>();
	square->half_width = square->half_height = s;
	square->color = entry.color;
	return square;
}

}

Point Legend::swatch_center( size_t index ) const
{
	// (across: the middle of its column; up and down: the line it is on, which the row is centered on)
	const Visual &swatch = *_swatches.at( index );
	return { point_at( swatch, Anchor::CENTER ).x() + _rows.at( index )->position.x(), swatch.position.y() + _rows.at( index )->position.y() };
}

AABB Legend::label_box( size_t index ) const
{
	const AABB box = box_of( *_labels.at( index ) );
	const Point at = _rows.at( index )->position;
	return { box.min + at, box.max + at };
}

std::unique_ptr<Legend> make_legend( Theme &theme, const std::vector<LegendEntry> &entries, const LegendOptions &options )
{
	auto legend = std::make_unique<Legend>();
	const float em = theme.get_stack( options.style ).font( 0 ).scale;

	// The swatches first: the names start after the widest, so they line up, and each swatch is centered in its column
	std::vector<std::unique_ptr<Visual>> swatches;
	float swatch_width = 0.F;
	for ( const LegendEntry &entry: entries )
	{
		swatches.push_back( make_swatch( entry, options ) );
		const AABB box = box_of( *swatches.back() );
		swatch_width = std::max( swatch_width, box.max.x() - box.min.x() );
	}

	// Each entry a row of its own: the swatch (its middle on the line the text is centered on), then the name
	auto content = std::make_unique<Group>();
	std::vector<Group *> rows;
	for ( size_t i = 0; i < entries.size(); ++i )
	{
		auto row = std::make_unique<Group>();
		auto &swatch = row->add( std::move( swatches[i] ) );
		const AABB swatch_box = box_of( swatch );
		swatch.position = swatch.position + Point{ swatch_width / 2.F - ( swatch_box.min.x() + swatch_box.max.x() ) / 2.F, 0.F };
		legend->_swatches.push_back( &swatch );

		auto &label = row->add( make_text( theme, entries[i].name, options.style ) );
		const AABB box = box_of( label );
		// The text starts a gap past the swatch column; its middle (of the x-height, about a third of an em) on the swatch's line
		label.position = { swatch_width + options.swatch_gap * em - box.min.x(), -0.3F * em };
		legend->_labels.push_back( &label );
		rows.push_back( &content->add( std::move( row ) ) );
		legend->_rows.push_back( rows.back() );
	}

	// Down by a fixed step, or along by each entry's width and a gap. Each row is a line tall, centered on its swatch's line, so the
	// room round the entries is even (the text's own box depends on its letters)
	const float half_row = LINE_HEIGHT * em / 2.F;
	float at = 0.F;
	AABB covered;
	for ( Group *row: rows )
	{
		const AABB box = box_of( *row );
		if ( options.horizontal )
		{
			// (a row's swatch column starts at its left, so a row is as wide as it reaches)
			row->position = { at, 0.F };
			covered.add( { at, half_row } );
			covered.add( { at + box.max.x(), -half_row } );
			at += box.max.x() + options.entry_gap * 4.F * em;
		}
		else
		{
			row->position = { 0.F, -at };
			covered.add( { 0.F, -at + half_row } );
			covered.add( { box.max.x(), -at - half_row } );
			at += ( LINE_HEIGHT + options.entry_gap ) * em;
		}
	}
	if ( !covered.empty() )
	{
		content->extent = covered;
	}

	if ( options.panel )
	{
		legend->_panel = &legend->add( make_panel( std::move( content ), options.panel_options ) );
	}
	else
	{
		legend->add( std::move( content ) );
	}
	return legend;
}

LegendEntry legend_entry( const Scatter &points )
{
	const ScatterOptions &o = points.options();
	return { o.name, points.series_color(), o.line == JoinStyle::NONE ? Swatch::MARKER : Swatch::LINE_MARKER, o.marker };
}

LegendEntry legend_entry( const FunctionGraph &graph )
{
	return { graph.name(), graph.line_color(), Swatch::LINE, MarkerShape::CIRCLE };
}

std::vector<LegendEntry> legend_entries( const BarChart &chart )
{
	const BarChartOptions &o = chart.options();
	std::vector<LegendEntry> entries;
	const bool by_category = o.color_by_category && o.series.size() == 1;
	if ( by_category )
	{
		for ( size_t c = 0; c < chart.category_count(); ++c )
		{
			if ( Rectangle *bar = chart.bar( 0, c ) )
			{
				entries.push_back( { c < o.categories.size() ? o.categories[c] : std::string(), bar->color, Swatch::SQUARE, MarkerShape::CIRCLE } );
			}
		}
		return entries;
	}
	for ( size_t s = 0; s < chart.series_count(); ++s )
	{
		entries.push_back( { o.series[s].name, chart.series_color( s ), Swatch::SQUARE, MarkerShape::CIRCLE } );
	}
	return entries;
}

Legend &add_legend( Axes &axes, std::unique_ptr<Legend> legend, Anchor corner, float inset )
{
	Legend &placed = axes.add( std::move( legend ) );
	const AABB area{ { axes.left() + inset, axes.bottom() + inset }, { axes.right() - inset, axes.top() - inset } };
	Point where = { ( area.min.x() + area.max.x() ) / 2.F, ( area.min.y() + area.max.y() ) / 2.F };
	switch ( corner )
	{
		case Anchor::TOP_LEFT: where = { area.min.x(), area.max.y() }; break;
		case Anchor::TOP_RIGHT: where = { area.max.x(), area.max.y() }; break;
		case Anchor::BOTTOM_LEFT: where = { area.min.x(), area.min.y() }; break;
		case Anchor::BOTTOM_RIGHT: where = { area.max.x(), area.min.y() }; break;
		case Anchor::TOP: where = { where.x(), area.max.y() }; break;
		case Anchor::BOTTOM: where = { where.x(), area.min.y() }; break;
		case Anchor::LEFT: where = { area.min.x(), where.y() }; break;
		case Anchor::RIGHT: where = { area.max.x(), where.y() }; break;
		default: break;
	}
	move_to( placed, where, corner );
	return placed;
}
