#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Palette.h>
#include <core/Stage.h>
#include <core/Theme.h>
#include <plot/Axes.h>
#include <plot/Graph.h>
#include <themes/AcademicClassic.h>
#include <themes/NewComputerModern.h>
#include <visuals/Morph.h>
#include <visuals/Rectangle.h>

#include <algorithm>
#include <sstream>

using Catch::Approx;

TEST_CASE( "Color::hex reads a hex color", "[palette]" )
{
	const Color c = Color::hex( 0x61AFEF );
	CHECK( c.r == Approx( 97.F / 255.F ) );
	CHECK( c.g == Approx( 175.F / 255.F ) );
	CHECK( c.b == Approx( 239.F / 255.F ) );
	CHECK( c.a == 1.F );
}

TEST_CASE( "Pure white stands for the text color, and nothing else does", "[palette]" )
{
	const Color dark = Color::hex( 0x1f2328 );

	// White is the text color, keeping its own alpha (a fade, a faint grid)
	const Color white_half = Color{ 1.F, 1.F, 1.F, 0.5F }.resolved( dark );
	CHECK( white_half.r == dark.r );
	CHECK( white_half.b == dark.b );
	CHECK( white_half.a == Approx( 0.5F ) );

	// Anything with a color of its own is left alone, even a hair from white
	const Color tint{ 1.F, 0.85F, 0.3F, 1.F };
	CHECK( tint.resolved( dark ) == tint );
	const Color almost{ 1.F, 1.F, 0.999F, 1.F };
	CHECK( almost.resolved( dark ) == almost );

	// And with a white foreground, the default, white is white
	CHECK( Color{ 1.F, 1.F, 1.F, 0.3F }.resolved( Color{ 1.F, 1.F, 1.F, 1.F } ) == Color{ 1.F, 1.F, 1.F, 0.3F } );
}

TEST_CASE( "Palettes are found by name, and an unknown one says which there are", "[palette]" )
{
	for ( const char *name: { "one-dark", "github-dark", "catppuccin-mocha", "academic", "blueprint", "github-light", "catppuccin-latte", "solarized-light", "paper" } )
	{
		INFO( name );
		REQUIRE( Palette::find( name ) );
		CHECK( Palette::find( name )->name == name );
	}
	CHECK( Palette::find( "nonesuch" ) == nullptr );

	try
	{
		Palette::get( "nonesuch" );
		FAIL( "should have thrown" );
	}
	catch ( const std::invalid_argument &e )
	{
		const std::string message = e.what();
		CHECK( message.find( "nonesuch" ) != std::string::npos );
		CHECK( message.find( "paper" ) != std::string::npos );
	}

	// The light ones are light, and the dark ones dark: what they are drawn on and what is drawn
	auto lightness = []( const Color &c ) { return 0.299F * c.r + 0.587F * c.g + 0.114F * c.b; };
	for ( const char *name: { "github-light", "catppuccin-latte", "solarized-light", "paper" } )
	{
		const Palette &p = Palette::get( name );
		CHECK( lightness( p.background ) > 0.85F );
		CHECK( lightness( p.text ) < 0.45F );
	}
	for ( const char *name: { "one-dark", "github-dark", "catppuccin-mocha", "academic", "blueprint" } )
	{
		const Palette &p = Palette::get( name );
		CHECK( lightness( p.background ) < 0.2F );
		CHECK( lightness( p.text ) > 0.6F );
	}
}

TEST_CASE( "A palette of your own can be added, and house() keeps its colors on black", "[palette]" )
{
	Palette mine = Palette::get( "paper" );
	mine.name = "my-palette";
	mine.blue = Color::hex( 0x123456 );
	Palette::add( mine );
	REQUIRE( Palette::find( "my-palette" ) );
	CHECK( Palette::get( "my-palette" ).blue == Color::hex( 0x123456 ) );

	const Palette house = Palette::house( "github-light" );
	CHECK( house.background == Color{ 0.F, 0.F, 0.F, 1.F } );
	CHECK( house.text == Palette::get( "github-light" ).text );
}

TEST_CASE( "The code colors come from a palette's accents", "[palette]" )
{
	const Palette &p = Palette::get( "one-dark" );
	CHECK( p.code.style_for( "keyword" ).color == p.purple );
	CHECK( p.code.style_for( "string" ).color == p.green );
	CHECK( p.code.style_for( "function" ).color == p.blue );
	CHECK( p.code.style_for( "function.special" ).color == p.blue ); // a kind of function
	CHECK( p.code.style_for( "comment" ).color == p.muted );
	CHECK( p.code.plain.color == p.text );

	// A palette can set its own apart from the derived: academic's keywords are bold
	CHECK( Palette::get( "academic" ).code.style_for( "keyword" ).weight == Weight::BOLD );
	CHECK_FALSE( Palette::get( "one-dark" ).code.style_for( "keyword" ).weight.has_value() );
}

TEST_CASE( "A theme keeps its own palette, on black, and any palette can be used with it", "[palette]" )
{
	Theme theme = AcademicClassic::create_theme();
	CHECK( theme.palette().name == "academic" );
	CHECK( theme.palette().background == Color{ 0.F, 0.F, 0.F, 1.F } );

	theme.set_palette( Palette::get( "solarized-light" ) );
	CHECK( theme.palette().name == "solarized-light" );
	CHECK( theme.code_palette().plain.color == Palette::get( "solarized-light" ).text );
}

namespace
{
std::string svg_of( const Stage &stage )
{
	std::ostringstream out;
	stage.save_svg( out );
	return out.str();
}
}

TEST_CASE( "A stage on a palette draws what has no color in its text color, on its background", "[palette]" )
{
	Stage stage;
	auto plain = std::make_unique<Rectangle>(); // white, as every visual is until it is given a color
	plain->half_width = plain->half_height = 1.F;
	stage.visuals.add( std::move( plain ) );
	auto colored = std::make_unique<Rectangle>();
	colored->half_width = colored->half_height = 0.5F;
	colored->color = Color::hex( 0xff8000 );
	stage.visuals.add( std::move( colored ) );

	// As it always was: white on black
	std::string svg = svg_of( stage );
	CHECK( svg.find( "fill=\"rgb(255,255,255)\"" ) != std::string::npos );

	stage.use_palette( Palette::get( "github-light" ) ); // white background, near black text
	svg = svg_of( stage );
	CHECK( svg.find( "fill=\"rgb(255,255,255)\" fill-opacity=\"1\" />\n  <g" ) != std::string::npos ); // the background
	CHECK( svg.find( "fill=\"rgb(31,35,40)\"" ) != std::string::npos );  // the plain one, in the text color
	CHECK( svg.find( "fill=\"rgb(255,128,0)\"" ) != std::string::npos ); // the colored one, as it is
}

TEST_CASE( "A morph blends what has no color as the color it is seen in", "[palette]" )
{
	Rectangle from;
	from.half_width = from.half_height = 1.F; // white: no color of its own
	Rectangle to;
	to.half_width = to.half_height = 1.F;
	to.color = Color::hex( 0xff0000 );

	const Color foreground = Color::hex( 0x202020 );
	MorphOptions options;
	options.foreground = foreground;
	Morph morph( from, to, options );

	// At the start it is the foreground, not white (which would flash a bright shape on a light background)
	morph.set( 0.F );
	const auto &path = dynamic_cast<const Path &>( **morph.visual().begin() );
	CHECK( path.color.r == Approx( foreground.r ) );
	morph.set( 0.1F );
	const auto &now = dynamic_cast<const Path &>( **morph.visual().begin() );
	CHECK( now.color.r < 0.5F ); // still dark, on its way to red
}

TEST_CASE( "Curves on the same axes take the palette's accents in turn", "[palette]" )
{
	Theme theme = NewComputerModern::create_theme();
	theme.set_palette( Palette::get( "github-light" ) );
	auto axes = make_axes( theme );
	const Palette &p = theme.palette();

	auto f = []( double x ) { return x; };
	auto first = plot( *axes, f );
	auto second = plot( *axes, f );
	CHECK( first->stroke_color == p.blue );
	CHECK( second->stroke_color == p.orange );

	// A color given is used, and does not use up one of the palette's
	GraphOptions own;
	own.color = Color::hex( 0x00ff00 );
	auto given = plot( *axes, f, own );
	CHECK( given->stroke_color == Color::hex( 0x00ff00 ) );
	CHECK( plot( *axes, f )->stroke_color == p.green ); // the third accent
}

TEST_CASE( "The axes are drawn in the text color, whatever the palette", "[palette]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto axes = make_axes( theme );
	CHECK( axes->options().color == Color{ 1.F, 1.F, 1.F, 1.F } ); // white: the text color, dark on a light background
}

TEST_CASE( "background_fill is the background, but never taken for the theme's text color", "[palette]" )
{
	Palette dark = Palette::get( "one-dark" );
	CHECK( dark.background_fill().r == dark.background.r );

	Palette white = dark;
	white.background = { 1.F, 1.F, 1.F, 1.F };
	const Color fill = white.background_fill();
	CHECK( fill.r > 0.99F );
	CHECK( fill.r < 1.F ); // (not pure white, which a shape's color would resolve to the text color)
	const Color text = { 0.1F, 0.1F, 0.1F, 1.F };
	CHECK( fill.resolved( text ).r == fill.r );
	CHECK( Color( 1.F, 1.F, 1.F, 1.F ).resolved( text ).r == text.r );
}
