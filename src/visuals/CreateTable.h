#pragma once

#include "visuals/Group.h"
#include "visuals/Placement.h"

#include <fonts/FontStyle.h>

#include <memory>
#include <string>
#include <vector>

class Theme;

struct TableOptions
{
	FontStyle style = FontStyle{ Intent::PROSE };

	// The first row is a header: in bold, and set off by a rule under it
	bool header = true;

	// Between the columns, and between the rows (the space between one row's lines and the next's), in ems: the size of the
	// text, so the table keeps its look at any size. Rows go down a fixed step, not by how tall their letters are.
	float column_gap = 2.5F;
	float row_spacing = 0.5F;

	// A line under the header, and (if grid) lines between every row and column. White: the theme's text color, faint.
	bool header_rule = true;
	bool grid = false;
	Color rule_color = { 1.F, 1.F, 1.F, 0.4F };
	float rule_width = 0.03F;

	// How each column's cells line up: START (left), CENTER or END (right); the last one given serves the rest
	std::vector<Align> alignment = { Align::START };
};

// A table of text: `rows` of cells (all rows the same length, or the short ones are left short). The cells of a row share a
// baseline, a column is as wide as its widest cell, and the rules (if any) are drawn behind. The children are the rules
// (one Group, first) and then the cells, row by row: cell( row, column ). The origin is the middle of the table.
class Table : public Group
{
public:
	Visual &cell( size_t row, size_t column ) const;

	size_t rows() const
	{
		return _rows;
	}

	size_t columns() const
	{
		return _columns;
	}

private:
	friend std::unique_ptr<Table> make_table( Theme &theme, const std::vector<std::vector<std::string>> &rows, const TableOptions &options );

	size_t _rows = 0;
	size_t _columns = 0;
	std::vector<std::vector<Visual *>> _cells;
};

std::unique_ptr<Table> make_table( Theme &theme, const std::vector<std::vector<std::string>> &rows, const TableOptions &options = {} );
