#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include <memory>
#include <vector>

enum class ColumnAlign
{
	LEFT,
	CENTER,
	RIGHT,
};

struct MatrixOptions
{
	std::vector<ColumnAlign> align = {};  // per column; columns beyond the list are centered
	float column_sep_em = 1.F;       // between columns (amsmath: twice \arraycolsep, 5pt)
	float baseline_skip_em = 1.2F;   // between row baselines (TeX's \baselineskip, 12pt at 10pt)
};

// A grid of cells (TeX's \matrix and array). Columns are as wide as their widest cell and
// aligned as `options.align` says. Each row is at least a strut tall (0.7 and 0.3 of the
// baseline skip), rows are a baseline skip apart, and taller rows are pushed apart by the
// smallest gap TeX allows (a tenth of an em). The whole grid is centered on the math axis, so
// delimiters around it (a Delimited) are symmetric. Cells are expected at text style.
//
// The grid lays itself out again on every append_row(). Its baseline is at y = 0 and its left
// edge at x = 0.
class Matrix : public Group
{
public:
	Matrix( const MathFont &font, float glyph_scale, MatrixOptions options = {} );

	// Cells left to right; a row may have fewer cells than the widest row.
	void append_row( std::vector<std::unique_ptr<Visual>> &&cells );

private:
	void layout();

	MatrixOptions _options;
	float _em = 0.F;
	float _axis = 0.F;
	std::vector<std::vector<Visual *>> _rows;
};

// The cells of a row: append_row( make_cells( a, b, c ) )
template<typename... Cells>
std::vector<std::unique_ptr<Visual>> make_cells( Cells &&...cells )
{
	std::vector<std::unique_ptr<Visual>> row;
	row.reserve( sizeof...( cells ) );
	( row.push_back( std::move( cells ) ), ... );
	return row;
}
