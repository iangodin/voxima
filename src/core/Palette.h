#pragma once

#include <code/CodePalette.h>
#include <visuals/Color.h>

#include <string>
#include <string_view>
#include <vector>

// The colors of a look, apart from its fonts (a Theme has both, and either can be changed: a theme's fonts with any
// palette). It names roles, not uses: what is drawn in a role's color is chosen where it is drawn, so the code's
// colors, a plot's, and the lines pointed out all come from the same few colors.
//
//     background, text        what the picture is on, and what is drawn on it by default
//     muted                   what is there but not the point: axes, grids, comments
//     eight accents           red orange yellow green cyan blue purple pink: the colors of what stands out
//
// Text is drawn in `text` where nothing else is chosen (Stage::foreground: pure white means the theme's text color), so the
// same scene reads on a dark background and on a light one. What has a color of its own keeps it: use a palette's accents
// for those, so they suit the background: theme->palette().blue.
struct Palette
{
	std::string name;

	Color background = { 0.F, 0.F, 0.F, 1.F };
	Color text = { 1.F, 1.F, 1.F, 1.F };
	Color muted = { 0.6F, 0.6F, 0.6F, 1.F };

	Color red, orange, yellow, green, cyan, blue, purple, pink;

	// How source code is colored (make_code), worked out from the accents by make_palette, and adjusted where a palette
	// wants: bold keywords, another color for strings
	CodePalette code;

	// The accents in the order a plot uses them for its curves: each new one is a color of its own, and it is the same
	// run of colors in every palette
	std::vector<Color> series() const
	{
		return { blue, orange, green, purple, red, cyan, yellow, pink };
	}

	// The background as a color to fill a shape with, so it shows as the background. Pure white in a shape means "the theme's text
	// color" (Color::resolved), so a palette whose background is pure white (github-light) would fill it with the text color instead:
	// this is that white, a hair off, which is not taken for it.
	Color background_fill() const
	{
		Color fill = background;
		if ( fill.r == 1.F && fill.g == 1.F && fill.b == 1.F && fill.a == 1.F )
		{
			fill = { 0.999F, 0.999F, 0.999F, 1.F };
		}
		return fill;
	}

	// The same palette on another background (a theme keeps its look on the black it always had: see house())
	Palette with_background( const Color &color ) const
	{
		Palette copy = *this;
		copy.background = color;
		return copy;
	}

	// ---- The palettes there are ----
	//
	// Dark: one-dark, github-dark, catppuccin-mocha, academic, blueprint. Light: github-light, catppuccin-latte,
	// solarized-light, paper.

	// A palette by name, or nothing
	static const Palette *find( std::string_view name );

	// A palette by name; an unknown name throws, and says which there are
	static const Palette &get( std::string_view name );

	// The names, in order
	static std::vector<std::string> names();

	// Add a palette of your own (or replace one): it can then be found by name, and given with --palette
	static void add( Palette palette );

	// A palette on black, as the themes have always had: the colors of `name` on the plain black background of
	// every video so far
	static Palette house( std::string_view name );
};

// The code colors that go with a palette's accents: keywords purple, strings green, numbers orange, types yellow,
// functions blue, properties red, operators cyan, comments muted and italic; the rest plain. The band behind
// pointed-out lines is a see-through blue.
CodePalette derive_code_palette( const Palette &palette );
