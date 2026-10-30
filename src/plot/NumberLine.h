#pragma once

#include <core/Theme.h>
#include <plot/Axes.h>
#include <visuals/Group.h>

#include <memory>
#include <optional>
#include <string>

struct NumberLineOptions
{
	Range range{ -5.0, 5.0 };
	float length = 12.F; // stage units, across the range

	// Distance between ticks in data units; 0 to choose one
	double step = 0.0;

	bool numbers = true;
	bool arrows = true;

	// White is the theme's text color
	Color color = { 1.F, 1.F, 1.F, 1.F };
	float thickness = 0.03F;
	float tick_length = 0.15F;
	FontStyle number_style = FontStyle{ Intent::LABEL }.small();
};

struct IntervalOptions
{
	// An end that is included is a filled dot; one that is not, a hollow one
	bool closed_from = true;
	bool closed_to = true;
	float thickness = 0.12F;
};

// A line with ticks and numbers: to mark a value, a range, or show how far things are apart. Its origin is the middle of the line.
//
//     auto &line = stage.visuals.add( make_number_line( theme, { .range = { 0, 10 } } ) );
//     line.add_mark( theme, 3.5, "x", palette.orange );
//     line.add_interval( 6, 9, palette.blue, { .closed_from = true, .closed_to = false } );   // [6, 9)
class NumberLine : public Group
{
public:
	explicit NumberLine( const NumberLineOptions &options );

	const NumberLineOptions &options() const
	{
		return _options;
	}

	// Where a value is, in the line's own coordinates (the value need not be in range)
	Point at( double value ) const;

	// A dot on the line at a value, with a label above it (none if empty)
	Group &add_mark( Theme &theme, double value, const std::string &label, const Color &color, const FontStyle &style = FontStyle{ Intent::LABEL } );

	// A thick stretch of the line from one value to another, with a dot at each end (a hollow one is the palette's background color)
	Group &add_interval( Theme &theme, double from, double to, const Color &color, const IntervalOptions &options = {} );

private:
	NumberLineOptions _options;
};

std::unique_ptr<NumberLine> make_number_line( Theme &theme, const NumberLineOptions &options = {} );
