#include "visuals/CreateTable.h"

#include "visuals/CreateText.h"
#include "visuals/Path.h"

#include <core/Theme.h>

#include <algorithm>

Visual &Table::cell( size_t row, size_t column ) const
{
	return *_cells.at( row ).at( column );
}

std::unique_ptr<Table> make_table( Theme &theme, const std::vector<std::vector<std::string>> &rows, const TableOptions &options )
{
	auto table = std::make_unique<Table>();
	table->_rows = rows.size();
	for ( const auto &row: rows )
	{
		table->_columns = std::max( table->_columns, row.size() );
	}
	const size_t columns = table->_columns;

	// One group for the rules, first, so they are behind the text
	Group &rules = table->add( std::make_unique<Group>() );

	FontStyle header_style = options.style;
	header_style.bold();

	const float em = theme.get_stack( options.style ).font( 0 ).scale;
	constexpr float LINE = 1.2F; // a line's height in ems: what make_text lays lines out at

	// The text of each cell, and its box (the text is at its own origin, baseline at y = 0)
	struct Made
	{
		Visual *visual = nullptr;
		AABB box;
	};
	std::vector<std::vector<Made>> made( rows.size() );
	std::vector<float> widths( columns, 0.F );
	std::vector<size_t> lines( rows.size(), 1 );
	for ( size_t r = 0; r < rows.size(); ++r )
	{
		table->_cells.emplace_back();
		for ( size_t c = 0; c < rows[r].size(); ++c )
		{
			auto &text = table->add( make_text( theme, rows[r][c], options.header && r == 0 ? header_style : options.style ) );
			const AABB box = box_of( text );
			made[r].push_back( { &text, box } );
			table->_cells[r].push_back( &text );
			if ( !box.empty() )
			{
				widths[c] = std::max( widths[c], box.max.x() - box.min.x() );
			}
			lines[r] = std::max( lines[r], static_cast<size_t>( std::count( rows[r][c].begin(), rows[r][c].end(), '\n' ) ) + 1 );
		}
	}

	// The columns' left edges, and the rows' first baselines (going down from 0, a fixed step for each)
	const float gap = options.column_gap * em;
	std::vector<float> lefts( columns, 0.F );
	for ( size_t c = 1; c < columns; ++c )
	{
		lefts[c] = lefts[c - 1] + widths[c - 1] + gap;
	}
	std::vector<float> baselines( rows.size(), 0.F );
	for ( size_t r = 1; r < rows.size(); ++r )
	{
		baselines[r] = baselines[r - 1] - ( static_cast<float>( lines[r - 1] ) * LINE + options.row_spacing ) * em;
	}

	for ( size_t r = 0; r < rows.size(); ++r )
	{
		for ( size_t c = 0; c < made[r].size(); ++c )
		{
			const Align align = options.alignment.empty() ? Align::START : options.alignment[std::min( c, options.alignment.size() - 1 )];
			const AABB &box = made[r][c].box;
			if ( box.empty() )
			{
				continue;
			}
			const float w = box.max.x() - box.min.x();
			float x = lefts[c];
			if ( align == Align::CENTER )
				x += ( widths[c] - w ) / 2.F;
			else if ( align == Align::END )
				x += widths[c] - w;
			made[r][c].visual->position = { x - box.min.x(), baselines[r] };
		}
	}

	// The rules: one path, so where they cross they are one stroke (see-through lines would blend twice). A rule goes
	// half way between the letters of one row (down to the descenders) and the next (up to the ascenders).
	const float total_width = columns == 0 ? 0.F : lefts.back() + widths.back();
	const float pad = gap / 2.F;
	auto last_baseline = [&]( size_t r ) { return baselines[r] - static_cast<float>( lines[r] - 1 ) * LINE * em; };
	auto between = [&]( size_t r ) { return ( last_baseline( r ) - 0.25F * em + baselines[r + 1] + 0.75F * em ) / 2.F; };
	const float top_edge = rows.empty() ? 0.F : baselines[0] + 0.75F * em + pad / 2.F;
	const float bottom_edge = rows.empty() ? 0.F : last_baseline( rows.size() - 1 ) - 0.25F * em - pad / 2.F;

	auto path = std::make_unique<Path>();
	path->color = { 1.F, 1.F, 1.F, 0.F }; // lines, not a shape
	path->stroke_color = options.rule_color;
	path->stroke_thickness = options.rule_width;
	auto horizontal = [&]( float y ) {
		path->commands.push_back( MoveTo{ { -pad, y } } );
		path->commands.push_back( LineTo{ { total_width + pad, y } } );
	};
	auto vertical = [&]( float x ) {
		path->commands.push_back( MoveTo{ { x, top_edge } } );
		path->commands.push_back( LineTo{ { x, bottom_edge } } );
	};
	if ( options.grid && !rows.empty() )
	{
		horizontal( top_edge );
		for ( size_t r = 0; r + 1 < rows.size(); ++r )
		{
			horizontal( between( r ) );
		}
		horizontal( bottom_edge );
		vertical( -pad );
		for ( size_t c = 0; c + 1 < columns; ++c )
		{
			vertical( lefts[c + 1] - pad );
		}
		vertical( total_width + pad );
	}
	else if ( options.header && options.header_rule && rows.size() > 1 )
	{
		horizontal( between( 0 ) );
	}
	if ( !path->commands.empty() )
	{
		rules.add( std::move( path ) );
	}

	// The middle of the table at the origin
	AABB covered;
	for ( const auto &row: made )
	{
		for ( const Made &m: row )
		{
			if ( m.visual && !box_of( *m.visual ).empty() )
			{
				covered.add( box_of( *m.visual ) );
			}
		}
	}
	if ( !covered.empty() )
	{
		const Point middle{ ( covered.min.x() + covered.max.x() ) / 2.F, ( covered.min.y() + covered.max.y() ) / 2.F };
		for ( const auto &child: *table )
		{
			child->position = child->position - middle;
		}
	}
	return table;
}
