#include "visuals/CreateTable.h"

#include "visuals/CreateText.h"
#include "visuals/Path.h"
#include "visuals/Rectangle.h"
#include "visuals/WrapText.h"

#include <core/Theme.h>

#include <algorithm>
#include <cmath>

Visual &Table::cell( size_t row, size_t column ) const
{
	return *_cells.at( row ).at( column );
}

namespace
{

constexpr float LINE = 1.2F; // a line's height in ems: what make_text lays lines out at

float width_of( Theme &theme, const std::string &text, const FontStyle &style )
{
	const AABB box = box_of( *make_text( theme, text, style ) );
	return box.empty() ? 0.F : box.max.x() - box.min.x();
}

// Column widths to fit `available` in all: the widest cut down to a common width, as little as it takes (none below `least`)
std::vector<float> narrowed( const std::vector<float> &natural, float available, float least )
{
	std::vector<float> widths = natural;
	auto total = [&]( float cap ) {
		float sum = 0.F;
		for ( float w: natural )
			sum += std::min( w, cap );
		return sum;
	};
	float top = *std::max_element( natural.begin(), natural.end() );
	if ( total( top ) <= available )
	{
		return widths;
	}
	float low = least;
	// The largest cap that fits (bisection: the total grows with the cap)
	if ( total( low ) >= available )
	{
		top = low;
	}
	else
	{
		for ( int i = 0; i < 40; ++i )
		{
			const float middle = ( low + top ) / 2.F;
			( total( middle ) <= available ? low : top ) = middle;
		}
		top = low;
	}
	for ( float &w: widths )
	{
		w = std::min( w, top );
	}
	return widths;
}

}

std::unique_ptr<Table> make_table( Theme &theme, const std::vector<std::vector<std::string>> &rows, const TableOptions &options )
{
	FontStyle header_style = options.style;
	header_style.bold();
	auto style_of = [&]( size_t r ) -> const FontStyle & { return options.header && r == 0 ? header_style : options.style; };

	std::vector<std::vector<std::string>> texts = rows;
	if ( options.max_width > 0.F )
	{
		const float em = theme.get_stack( options.style ).font( 0 ).scale;
		size_t columns = 0;
		for ( const auto &row: rows )
			columns = std::max( columns, row.size() );

		// How wide each column is as it is (its longest line), and how wide it may be
		std::vector<float> natural( columns, 0.F );
		for ( size_t r = 0; r < rows.size(); ++r )
			for ( size_t c = 0; c < rows[r].size(); ++c )
				natural[c] = std::max( natural[c], width_of( theme, rows[r][c], style_of( r ) ) );
		if ( columns > 0 )
		{
			const float available = options.max_width - options.column_gap * em * static_cast<float>( columns - 1 );
			const std::vector<float> widths = narrowed( natural, available, 4.F * em );
			for ( size_t r = 0; r < texts.size(); ++r )
				for ( size_t c = 0; c < texts[r].size(); ++c )
					if ( widths[c] < natural[c] )
						texts[r][c] = wrap_lines( theme, texts[r][c], style_of( r ), widths[c] );
		}
	}

	std::vector<std::vector<std::unique_ptr<Visual>>> cells( texts.size() );
	for ( size_t r = 0; r < texts.size(); ++r )
		for ( const std::string &text: texts[r] )
			cells[r].push_back( make_text( theme, text, style_of( r ) ) );
	return make_table( theme, std::move( cells ), options );
}

std::unique_ptr<Table> make_table( Theme &theme, std::vector<std::vector<std::unique_ptr<Visual>>> cells, const TableOptions &options )
{
	auto table = std::make_unique<Table>();
	table->_rows = cells.size();
	for ( const auto &row: cells )
	{
		table->_columns = std::max( table->_columns, row.size() );
	}
	const size_t columns = table->_columns;
	const size_t row_count = cells.size();

	// One group for the rules (and stripes), first, so they are behind the text
	Group &rules = table->add( std::make_unique<Group>() );

	const float em = theme.get_stack( options.style ).font( 0 ).scale;

	// Each cell, and its box (a cell is at its own origin, its baseline at y = 0)
	struct Made
	{
		Visual *visual = nullptr;
		AABB box;
	};
	std::vector<std::vector<Made>> made( row_count );
	std::vector<float> widths( columns, 0.F );
	std::vector<size_t> lines( row_count, 1 );
	for ( size_t r = 0; r < row_count; ++r )
	{
		table->_cells.emplace_back();
		for ( size_t c = 0; c < cells[r].size(); ++c )
		{
			auto &cell = table->add( std::move( cells[r][c] ) );
			const AABB box = box_of( cell );
			made[r].push_back( { &cell, box } );
			table->_cells[r].push_back( &cell );
			if ( !box.empty() )
			{
				widths[c] = std::max( widths[c], box.max.x() - box.min.x() );
				// How many lines of text it is as tall as
				const float height = ( box.max.y() - box.min.y() ) / ( LINE * em );
				lines[r] = std::max( lines[r], static_cast<size_t>( std::max( 1.F, std::ceil( height - 0.35F ) ) ) );
			}
		}
	}

	// The columns' left edges, and the rows' first baselines (going down from 0, a fixed step for each)
	const float gap = options.column_gap * em;
	std::vector<float> lefts( columns, 0.F );
	for ( size_t c = 1; c < columns; ++c )
	{
		lefts[c] = lefts[c - 1] + widths[c - 1] + gap;
	}
	std::vector<float> baselines( row_count, 0.F );
	for ( size_t r = 1; r < row_count; ++r )
	{
		baselines[r] = baselines[r - 1] - ( static_cast<float>( lines[r - 1] ) * LINE + options.row_spacing ) * em;
	}

	for ( size_t r = 0; r < row_count; ++r )
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
	const float top_edge = ( row_count == 0 ) ? 0.F : baselines[0] + 0.75F * em + pad / 2.F;
	const float bottom_edge = ( row_count == 0 ) ? 0.F : last_baseline( row_count - 1 ) - 0.25F * em - pad / 2.F;

	// The tints, behind the lines: each row's band runs from the middle of the gap above it to the middle of the gap below
	if ( row_count > 0 && ( options.striped || options.header_fill ) )
	{
		for ( size_t r = 0; r < row_count; ++r )
		{
			const bool header = options.header && r == 0;
			const size_t body = r - ( options.header ? 1 : 0 );
			std::optional<Color> tint;
			if ( header )
				tint = options.header_fill;
			else if ( options.striped && body % 2 == 1 )
				tint = options.stripe_color;
			if ( !tint )
				continue;
			const float top = r == 0 ? top_edge : between( r - 1 );
			const float bottom = r + 1 == row_count ? bottom_edge : between( r );
			auto band = std::make_unique<Rectangle>();
			band->half_width = ( total_width + 2.F * pad ) / 2.F;
			band->half_height = ( top - bottom ) / 2.F;
			band->position = { total_width / 2.F, ( top + bottom ) / 2.F };
			band->color = *tint;
			rules.add( std::move( band ) );
		}
	}

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
	if ( options.grid && !( row_count == 0 ) )
	{
		horizontal( top_edge );
		for ( size_t r = 0; r + 1 < row_count; ++r )
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
	else if ( options.header && options.header_rule && row_count > 1 )
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
