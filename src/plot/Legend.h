#pragma once

#include "BarChart.h"
#include "Graph.h"
#include "Scatter.h"

#include <core/Theme.h>
#include <visuals/Group.h>
#include <visuals/Panel.h>
#include <visuals/Placement.h>

#include <memory>
#include <string>
#include <vector>

// What a legend shows beside a name
enum class Swatch
{
	SQUARE,      // a small filled square: bars, areas
	LINE,        // a short line: a graph
	MARKER,      // a marker: points
	LINE_MARKER  // a short line with a marker on it: points joined by a line
};

struct LegendEntry
{
	std::string name;
	Color color = { 1.F, 1.F, 1.F, 1.F };
	Swatch swatch = Swatch::SQUARE;
	MarkerShape marker = MarkerShape::CIRCLE; // for MARKER and LINE_MARKER
};

struct LegendOptions
{
	// Entries one under another (the default) or side by side
	bool horizontal = false;

	// In a panel, a faint tint with an outline (see Panel.h), or bare
	bool panel = true;
	PanelOptions panel_options = { .padding = 0.25F, .radius = 0.15F, .outline_width = 0.02F };

	FontStyle style = FontStyle{ Intent::LABEL }.small();

	// The swatch's half-size (stage units), then, in ems of the text: the space between a swatch and its name, and between entries
	float swatch_size = 0.13F;
	float swatch_gap = 0.5F;
	float entry_gap = 0.35F;
};

// A key to a chart (make_legend): a swatch and a name for each entry, in a panel. A Group whose children are the panel (if there
// is one; the entries are inside it), so it is faded, moved or drawn on as one. Entries are found again by index.
class Legend : public Group
{
public:
	Legend() = default;

	size_t entry_count() const
	{
		return _swatches.size();
	}

	Visual &swatch( size_t index ) const
	{
		return *_swatches.at( index );
	}

	Visual &label( size_t index ) const
	{
		return *_labels.at( index );
	}

	// Where an entry's swatch is centered, and the box of its name, in the legend's own frame
	Point swatch_center( size_t index ) const;
	AABB label_box( size_t index ) const;

	// The panel behind the entries, or null when there is none
	Panel *panel() const
	{
		return _panel;
	}

private:
	friend std::unique_ptr<Legend> make_legend( Theme &theme, const std::vector<LegendEntry> &entries, const LegendOptions &options );

	Panel *_panel = nullptr;
	std::vector<Visual *> _swatches;
	std::vector<Visual *> _labels;
	std::vector<Group *> _rows;
};

std::unique_ptr<Legend> make_legend( Theme &theme, const std::vector<LegendEntry> &entries, const LegendOptions &options = {} );

// The entries for what is plotted, in the colors it is drawn in: a name for each series that has one (an unnamed series is left out)
LegendEntry legend_entry( const Scatter &points );
LegendEntry legend_entry( const FunctionGraph &graph );

// One entry for each series of a chart of bars (or, for one series with a color to each category, each category)
std::vector<LegendEntry> legend_entries( const BarChart &chart );

// A legend in a corner of the plot area of some axes (in the axes' own frame, so it goes with them): `corner` is one of
// TOP_LEFT, TOP_RIGHT, BOTTOM_LEFT, BOTTOM_RIGHT, or TOP, BOTTOM, LEFT, RIGHT for the middle of a side; `inset` is the room left
// between it and the edge of the plot. Drawn over what is plotted.
Legend &add_legend( Axes &axes, std::unique_ptr<Legend> legend, Anchor corner = Anchor::TOP_RIGHT, float inset = 0.2F );
