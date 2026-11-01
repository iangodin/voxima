#include "visuals/Matrix.h"
#include "visuals/Tags.h"
#include "visuals/MathMetrics.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <string>

namespace
{
constexpr float STRUT_HEIGHT = 0.7F; // of the baseline skip (TeX's \strut)
constexpr float STRUT_DEPTH = 0.3F;
constexpr float LINESKIP_EM = 0.1F;  // TeX's \lineskip, 1pt at 10pt
}

Matrix::Matrix( const MathFont &font, float glyph_scale, MatrixOptions options )
	: _options( std::move( options ) ),
	  _em( glyph_scale * static_cast<float>( font.face().face->units_per_EM ) ),
	  _axis( font.axis_height() * glyph_scale )
{
	math_axis = _axis;
}

void Matrix::append_row( std::vector<std::unique_ptr<Visual>> &&cells )
{
	std::vector<Visual *> row;
	row.reserve( cells.size() );
	const size_t r = _rows.size();
	for ( size_t c = 0; c < cells.size(); ++c )
	{
		Visual &cell = add( std::move( cells[c] ) );
		tag_visual( cell, "cell" );
		tag_visual( cell, "r" + std::to_string( r ) + "c" + std::to_string( c ) );
		row.push_back( &cell );
	}
	_rows.push_back( std::move( row ) );
	layout();
}

void Matrix::layout()
{
	extent.reset();
	if ( _rows.empty() )
	{
		return;
	}

	size_t columns = 0;
	for ( const auto &row: _rows )
	{
		columns = std::max( columns, row.size() );
	}

	const float baseline_skip = _options.baseline_skip_em * _em;
	const float lineskip = LINESKIP_EM * _em;

	// Measure: column widths, and each row's height and depth about its baseline
	std::vector<float> column_width( columns, 0.F );
	std::vector<float> row_height( _rows.size(), STRUT_HEIGHT * baseline_skip );
	std::vector<float> row_depth( _rows.size(), STRUT_DEPTH * baseline_skip );
	for ( size_t r = 0; r < _rows.size(); ++r )
	{
		for ( size_t c = 0; c < _rows[r].size(); ++c )
		{
			const ScriptBox cell = measure_box( *_rows[r][c] );
			column_width[c] = std::max( column_width[c], cell.box.width() );
			row_height[r] = std::max( row_height[r], cell.height() );
			row_depth[r] = std::max( row_depth[r], cell.depth() );
		}
	}

	// Columns left to right
	std::vector<float> column_left( columns, 0.F );
	float x = 0.F;
	for ( size_t c = 0; c < columns; ++c )
	{
		column_left[c] = x;
		x += column_width[c];
		if ( c + 1 < columns )
		{
			x += _options.column_sep_em * _em;
		}
	}
	const float total_width = x;

	// Row baselines, top to bottom: a baseline skip apart, or as close as the rows allow
	std::vector<float> baseline( _rows.size(), 0.F );
	for ( size_t r = 1; r < _rows.size(); ++r )
	{
		const float reach = row_depth[r - 1] + row_height[r];
		baseline[r] = baseline[r - 1] - ( baseline_skip >= reach ? baseline_skip : reach + lineskip );
	}

	// Center the whole grid on the math axis
	const float top = baseline.front() + row_height.front();
	const float bottom = baseline.back() - row_depth.back();
	const float shift = _axis - ( top + bottom ) / 2.F;

	for ( size_t r = 0; r < _rows.size(); ++r )
	{
		for ( size_t c = 0; c < _rows[r].size(); ++c )
		{
			Visual &cell = *_rows[r][c];
			const ScriptBox m = measure_box( cell );

			const ColumnAlign align = c < _options.align.size() ? _options.align[c] : ColumnAlign::CENTER;
			float left = column_left[c];
			if ( align == ColumnAlign::CENTER )
			{
				left += ( column_width[c] - m.box.width() ) / 2.F;
			}
			else if ( align == ColumnAlign::RIGHT )
			{
				left += column_width[c] - m.box.width();
			}

			cell.position = cell.position.movedBy( left - m.box.min.x(), baseline[r] + shift - m.baseline );
		}
	}

	AABB box;
	box.add( { 0.F, bottom + shift } );
	box.add( { total_width, top + shift } );
	extent = box;
}
