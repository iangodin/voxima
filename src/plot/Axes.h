#pragma once

#include <fonts/FontStyle.h>
#include <visuals/Color.h>
#include <visuals/Group.h>
#include <visuals/Point.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

class Theme;

// A stretch of numbers, from min to max
struct Range
{
	double min = 0.0;
	double max = 1.0;

	double span() const
	{
		return max - min;
	}
};

// Where the axes are
enum class AxisPlacement
{
	// Crossing at the origin, in the middle of the plot when it is in range (a sine wave's), and along the edge
	// otherwise. The origin is not ticked: its two numbers would run together.
	ORIGIN,

	// Along the bottom and the left edge, as most plotting libraries have them: the numbers are outside the plot, and
	// out of the way of the curves
	EDGE
};

// A tick with a text of its own in place of its number: the name of a category, a year, "π"
struct TickLabel
{
	double value = 0.0;
	std::string text;
};

struct AxesOptions
{
	AxisPlacement placement = AxisPlacement::ORIGIN;

	// The data shown, and how big the plot area is on the stage (in stage units: the stage is 16 wide)
	Range x{ -5.0, 5.0 };
	Range y{ -3.0, 3.0 };
	float width = 10.F;
	float height = 6.F;

	// The distance between ticks, in data units; 0 to choose one (a 1, 2 or 5 times a power of 10, for about one
	// tick a stage unit)
	double x_step = 0.0;
	double y_step = 0.0;

	// Ticks at chosen places with chosen texts, in place of the ticks at even steps and their numbers (an axis with none of these
	// has the numbers as before). The values are in data units.
	std::vector<TickLabel> x_tick_labels = {};
	std::vector<TickLabel> y_tick_labels = {};

	// Numbers at the ticks, arrow tips at the positive ends, faint lines across at every tick
	bool numbers = true;
	bool arrows = true;
	bool grid = false;

	// With a grid: lines at the x ticks (up and down), and lines at the y ticks (across). A chart of bars has only the second.
	bool grid_x = true;
	bool grid_y = true;

	// What the axes are called, at their ends, in math italic ("" for none)
	std::string x_label = "x";
	std::string y_label = "y";

	// White is the theme's text color (see Color::resolved): the axes, their ticks and numbers, and the faint grid,
	// are dark on a light background and light on a dark one, whatever the palette
	Color color = { 1.F, 1.F, 1.F, 1.F };
	Color grid_color = { 1.F, 1.F, 1.F, 0.18F };
	float thickness = 0.03F;
	float tick_length = 0.12F;
	FontStyle number_style = FontStyle{ Intent::LABEL }.small();

	// An outline in the background's color around each number, so a curve that passes through it leaves it readable
	// (for the stage's black: Color{ 0, 0, 0, 1 }). None by default: the numbers are drawn over the curves as they are.
	std::optional<Color> number_halo = {};
	float halo_width = 0.08F;
	FontStyle label_style = FontStyle{ Intent::MATH };
};

// A pair of axes (make_axes): a Group whose origin is the middle of the plot area. Add what is plotted on them with
// add_plot (see plot()), so it moves and scales with them. From the bottom up they are drawn: the grid, the axes with
// their ticks and arrow tips, what is plotted, then the numbers and names (each number with a halo, so it can be read
// where a curve passes over it). (Group::add puts a visual on top of everything instead.)
// The tags on the parts of axes (see Tags.h), for animating them apart: the axis lines and their arrow tips; each tick mark and its
// number (and the halo under it) share a tag "tick-N" (N counting the ticks from 0, x's first) besides their own kind; the names
// at the ends; and the grid, which is the first child of the axes.
inline constexpr const char *AXIS_LINE_TAG = "axis-line";
inline constexpr const char *AXIS_ARROW_TAG = "axis-arrow";
inline constexpr const char *TICK_TAG = "tick";
inline constexpr const char *NUMBER_TAG = "number";
inline constexpr const char *NUMBER_HALO_TAG = "number-halo";
inline constexpr const char *AXIS_NAME_TAG = "axis-name";

class Axes : public Group
{
public:
	explicit Axes( const AxesOptions &options );

	const AxesOptions &options() const
	{
		return _options;
	}

	// Add something plotted: over the grid and the axes, under the numbers and names
	template<typename Shape>
	Shape &add_plot( std::unique_ptr<Shape> &&visual )
	{
		return _plots->add( std::move( visual ) );
	}

	// Where the data point (x, y) is, in the axes' own coordinates (x and y need not be in range)
	Point at( double x, double y ) const;

	// The plot area, in the axes' own coordinates
	float left() const
	{
		return -_options.width / 2.F;
	}
	float right() const
	{
		return _options.width / 2.F;
	}
	float bottom() const
	{
		return -_options.height / 2.F;
	}
	float top() const
	{
		return _options.height / 2.F;
	}

	// The next color for a curve, from the palette's accents (blue, orange, green ...): each plot without a color of its
	// own gets the next, so curves on the same axes are told apart, in the same colors in every palette
	Color next_series_color() const
	{
		if ( _series.empty() )
		{
			return { 1.F, 1.F, 1.F, 1.F };
		}
		return _series[_next_series++ % _series.size()];
	}

	// The data values ticked on each axis
	const std::vector<double> &x_ticks() const
	{
		return _x_ticks;
	}
	const std::vector<double> &y_ticks() const
	{
		return _y_ticks;
	}

	// Where the y axis is on x, and the x axis on y, in data units: for ORIGIN at 0 when that is in range, and
	// otherwise, and for EDGE, at the low end
	double x_crossing() const;
	double y_crossing() const;

private:
	friend std::unique_ptr<Axes> make_axes( Theme &theme, const AxesOptions &options );

	AxesOptions _options;
	Group *_grid = nullptr;  // the bottom layer
	Group *_lines = nullptr; // the axes, ticks and arrow tips
	Group *_plots = nullptr; // the layer for add_plot
	std::vector<Color> _series;
	mutable size_t _next_series = 0;
	std::vector<double> _x_ticks;
	std::vector<double> _y_ticks;
};

// Axes with ticks and numbers, as the options say
std::unique_ptr<Axes> make_axes( Theme &theme, const AxesOptions &options = {} );

// A tick distance for a span: 1, 2 or 5 times a power of 10, giving about `count` ticks
double nice_step( double span, double count );

// The multiples of `step` in the range (and 0 is exactly 0)
std::vector<double> tick_values( const Range &range, double step );

// A tick's number, with as many decimals as the step needs, and a real minus sign
std::string tick_text( double value, double step );
