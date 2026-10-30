#pragma once

#include "Axes.h"

#include <core/Theme.h>
#include <visuals/Group.h>
#include <visuals/Rectangle.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

struct BarSeries
{
	// What the series is called, for a legend
	std::string name;

	// One value for each category (a short series leaves the last categories without a bar)
	std::vector<double> values;

	// By default the next of the palette's accents, as plotted graphs take theirs
	std::optional<Color> color;
};

struct BarChartOptions
{
	std::vector<std::string> categories;
	std::vector<BarSeries> series;

	// Several series side by side in each category (the default), or stacked, one on top of the next
	bool stacked = false;

	// Bars going across, the categories down the left (the first at the top), instead of up
	bool horizontal = false;

	// The size of the plot area, in stage units
	float width = 10.F;
	float height = 6.F;

	// How much of each category's room the bars take, and the space between the bars of a group (a fraction of a bar's width)
	float bar_fraction = 0.7F;
	float bar_gap = 0.1F;

	// The values shown, by default from 0 (or the lowest value below it) up to the highest, rounded out to the tick steps
	std::optional<Range> values;
	double value_step = 0.0; // 0 to choose

	// The number on each bar (in each part, for stacked bars)
	bool value_labels = false;

	// Lines across the chart at the value ticks
	bool grid = true;

	// Each category's bars in a color of its own (the palette's accents in turn), where there is only one series
	bool color_by_category = false;

	// White is the theme's text color
	Color color = { 1.F, 1.F, 1.F, 1.F };
	FontStyle label_style = FontStyle{ Intent::LABEL }.small();
};

// A bar chart (make_bar_chart): a Group of its axes (axes(): the value axis with numbers, and the category names along the other
// side) with the bars on them. Bars are found by series and category, so they can be shown one after another, grown, or colored.
class BarChart : public Group
{
public:
	explicit BarChart( const BarChartOptions &options );

	const BarChartOptions &options() const
	{
		return _options;
	}

	Axes &axes() const
	{
		return *_axes;
	}

	size_t series_count() const
	{
		return _bars.size();
	}

	size_t category_count() const
	{
		return _options.categories.size();
	}

	// The bar of a series in a category, or null where the series has no value there
	Rectangle *bar( size_t series, size_t category ) const;

	// The color a series is drawn in
	Color series_color( size_t series ) const
	{
		return _colors.at( series );
	}

	// The range of values on the value axis (as chosen, or as the options gave it)
	const Range &value_range() const
	{
		return _range;
	}

private:
	friend std::unique_ptr<BarChart> make_bar_chart( Theme &theme, const BarChartOptions &options );

	BarChartOptions _options;
	Axes *_axes = nullptr;
	Range _range;
	std::vector<Color> _colors;
	std::vector<std::vector<Rectangle *>> _bars; // [series][category]
};

std::unique_ptr<BarChart> make_bar_chart( Theme &theme, const BarChartOptions &options );
