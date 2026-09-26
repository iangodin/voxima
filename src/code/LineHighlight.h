#pragma once

#include "CreateCode.h"

#include <core/Ease.h>
#include <core/EaseCubic.h>
#include <core/Scene.h>

#include <chrono>
#include <cstddef>
#include <initializer_list>
#include <optional>
#include <utility>
#include <vector>

struct Stage;

// Some lines of a listing, numbered from 1 as an editor numbers them: one line, a range, or several of either.
//
//     Lines( 5 )                              // line 5
//     Lines( 9, 13 )                          // lines 9 to 13
//     Lines{ { 3, 4 }, { 9, 13 } }            // lines 3 and 4, and 9 to 13
//     Lines( 3 ).add( 7, 8 ).add( 12 )        // the same sort of thing, built up
//
// Lines outside the listing are ignored when they are used, and lines given twice count once.
class Lines
{
public:
	Lines() = default;

	explicit Lines( size_t line ) :
		Lines( line, line )
	{
	}

	Lines( size_t first, size_t last )
	{
		add( first, last );
	}

	Lines( std::initializer_list<std::pair<size_t, size_t>> ranges )
	{
		for ( const auto &[first, last]: ranges )
		{
			add( first, last );
		}
	}

	Lines &add( size_t line )
	{
		return add( line, line );
	}

	Lines &add( size_t first, size_t last );

	bool empty() const
	{
		return _runs.empty();
	}

	bool contains( size_t line ) const;

	// The lines as runs of neighbouring lines, in order, none touching another: {3, 4, 5 and 6} is one run, 3 to 6
	const std::vector<std::pair<size_t, size_t>> &runs() const
	{
		return _runs;
	}

	bool operator==( const Lines & ) const = default;

private:
	std::vector<std::pair<size_t, size_t>> _runs;
};

struct HighlightOptions
{
	// How much of the text of the lines that are not pointed out is left (1: all of it, so they are not dimmed)
	float dim = 0.35F;

	// The band behind the lines pointed out: the theme's code palette gives its color (Theme is not asked: give
	// the color here to use another); with `bar` false, there is none and only the dimming shows
	std::optional<Color> band;
	bool bar = true;

	// How far the band goes past the text on each side, in ems
	float padding = 0.4F;
};

// Points at some lines of a listing (make_code): the others are dimmed, and a band goes behind them. It is set to
// some lines, or moves to others: the band slides from one place to the next (the runs, in order, go to the runs),
// and the dimming fades. With no lines, nothing is dimmed and there is no band.
//
//     LineHighlight pointer( listing, *stage.theme );
//     co_await pointer.to( stage, Lines( 5 ), 0.4s );          // in on line 5
//     co_await pointer.to( stage, Lines( 9, 13 ), 0.4s );      // over to lines 9 to 13
//     co_await pointer.to( stage, Lines{ { 3, 4 }, { 9, 13 } }, 0.4s );
//     co_await pointer.to( stage, Lines(), 0.4s );             // and away again
//
// Like the Typewriter and the Morph it is set to a point (blend), so anything can drive it: its own to(), a tween, the
// narration. The listing must stay alive (on the stage) while it is used.
class LineHighlight
{
public:
	LineHighlight( CodeListing &listing, const Theme &theme, const HighlightOptions &options = {} );

	// At these lines at once
	void set( const Lines &lines )
	{
		blend( lines, lines, 0.F );
	}

	// A fraction t of the way from lines a to lines b (0 to 1; beyond, as an ease may overshoot, it carries on)
	void blend( const Lines &a, const Lines &b, float t );

	// Move from where it is now to these lines over a length of time
	Scene to( const Stage &stage, Lines lines, std::chrono::duration<double> duration, EaseFunction ease = ease_cubic_in_out );

	const Lines &lines() const
	{
		return _at;
	}

private:
	struct Band
	{
		float top = 0.F;
		float bottom = 0.F;
		float alpha = 0.F;
	};

	// Where the band of a run of lines (from 1) is
	Band band_of( std::pair<size_t, size_t> run ) const;

	// The runs of lines the listing has: those past its end left out, the one that crosses it cut off there
	static std::vector<std::pair<size_t, size_t>> clipped( const Lines &lines, size_t count );

	CodeListing &_listing;
	HighlightOptions _options;
	Color _band_color;
	Lines _at; // where it is set to, or where it has got to
	std::vector<Rectangle *> _bands;
};
