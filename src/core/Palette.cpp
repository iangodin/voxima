#include "Palette.h"

#include <map>
#include <stdexcept>

CodePalette derive_code_palette( const Palette &p )
{
	CodePalette code;
	code.plain = { p.text };
	code.line_highlight = { p.blue.r, p.blue.g, p.blue.b, 0.16F };

	code.set( "comment", { p.muted, std::nullopt, Slant::ITALIC } );
	code.set( "keyword", { p.purple } );
	code.set( "string", { p.green } );
	code.set( "number", { p.orange } );
	code.set( "constant", { p.orange } );
	code.set( "type", { p.yellow } );
	code.set( "function", { p.blue } );
	code.set( "property", { p.red } );
	code.set( "variable", { p.text } );
	code.set( "variable.builtin", { p.red } );
	code.set( "label", { p.red } );
	code.set( "operator", { p.cyan } );
	code.set( "delimiter", { p.text } );
	return code;
}

namespace
{

// A palette from its colors, written as hex: background, text, muted, then the accents red, orange, yellow, green, cyan,
// blue, purple, pink, and its code colors derived from them
Palette make( std::string name, unsigned bg, unsigned text, unsigned muted, unsigned red, unsigned orange, unsigned yellow, unsigned green, unsigned cyan, unsigned blue, unsigned purple, unsigned pink )
{
	Palette p;
	p.name = std::move( name );
	p.background = Color::hex( bg );
	p.text = Color::hex( text );
	p.muted = Color::hex( muted );
	p.red = Color::hex( red );
	p.orange = Color::hex( orange );
	p.yellow = Color::hex( yellow );
	p.green = Color::hex( green );
	p.cyan = Color::hex( cyan );
	p.blue = Color::hex( blue );
	p.purple = Color::hex( purple );
	p.pink = Color::hex( pink );
	p.code = derive_code_palette( p );
	return p;
}

std::map<std::string, Palette, std::less<>> &registry()
{
	static std::map<std::string, Palette, std::less<>> palettes = []
	{
		std::map<std::string, Palette, std::less<>> all;
		auto add = [&all]( Palette p )
		{
			const std::string name = p.name;
			all.emplace( name, std::move( p ) );
		};

		// ---- Dark ----

		add( make( "one-dark", 0x282c34, 0xabb2bf, 0x7f848e, 0xe06c75, 0xd19a66, 0xe5c07b, 0x98c379, 0x56b6c2, 0x61afef, 0xc678dd, 0xe88ac9 ) );

		// Modern: GitHub's dark theme. Code as GitHub sets it: keywords coral, strings light blue, functions violet.
		{
			Palette p = make( "github-dark", 0x0d1117, 0xe6edf3, 0x8b949e, 0xff7b72, 0xffa657, 0xe3b341, 0x7ee787, 0xa5d6ff, 0x79c0ff, 0xd2a8ff, 0xf778ba );
			p.code.set( "keyword", { p.red } );
			p.code.set( "string", { p.cyan } );
			p.code.set( "number", { p.blue } );
			p.code.set( "constant", { p.blue } );
			p.code.set( "type", { p.orange } );
			p.code.set( "function", { p.purple } );
			p.code.set( "property", { p.green } );
			p.code.set( "operator", { p.red } );
			p.code.set( "variable.builtin", { p.blue } );
			p.code.set( "label", { p.orange } );
			p.code.set( "comment", { p.muted } ); // (a code font without an italic, like Fira Mono, shows it upright anyway)
			p.code.line_highlight = { 0.22F, 0.55F, 0.99F, 0.18F };
			add( std::move( p ) );
		}

		// Playful: Catppuccin Mocha, soft pastels
		{
			Palette p = make( "catppuccin-mocha", 0x1e1e2e, 0xcdd6f4, 0x9399b2, 0xf38ba8, 0xfab387, 0xf9e2af, 0xa6e3a1, 0x94e2d5, 0x89b4fa, 0xcba6f7, 0xf5c2e7 );
			p.code.set( "property", { p.pink } );
			p.code.line_highlight = { 0.71F, 0.75F, 1.F, 0.16F }; // lavender
			add( std::move( p ) );
		}

		// Academic: a printed textbook's listing, warm muted inks, bold keywords
		{
			Palette p = make( "academic", 0x1c1a17, 0xe8e2d0, 0x8c8577, 0xd7897f, 0xd6a86c, 0xc9b37e, 0x9cbf8a, 0x8fbfbf, 0x9db4d0, 0xc9a0c8, 0xd98fa8 );
			p.code.set( "keyword", { p.red, Weight::BOLD } );
			p.code.set( "type", { p.yellow } );
			p.code.set( "property", { p.purple } );
			p.code.set( "variable.builtin", { p.red } );
			p.code.set( "label", { p.purple } );
			p.code.set( "operator", { p.text } );
			p.code.set( "delimiter", { p.text } );
			p.code.line_highlight = { 0.84F, 0.66F, 0.42F, 0.18F };
			add( std::move( p ) );
		}

		// Blueprint: cyan and white on the dark, amber for literals, bold function names
		{
			Palette p = make( "blueprint", 0x0d1b2a, 0xd6e4f0, 0x6a8ba8, 0xef9a9a, 0xffab40, 0xffd54f, 0x80cbc4, 0x4fc3f7, 0x81d4fa, 0xb39ddb, 0xf48fb1 );
			p.code.set( "keyword", { p.cyan } );
			p.code.set( "string", { p.yellow } );
			p.code.set( "type", { p.green } );
			p.code.set( "function", { Color{ 1.F, 1.F, 1.F, 1.F }, Weight::BOLD } );
			p.code.set( "property", { p.blue } );
			p.code.set( "operator", { p.cyan } );
			p.code.set( "variable.builtin", { p.cyan } );
			p.code.set( "label", { p.orange } );
			p.code.line_highlight = { 0.31F, 0.76F, 0.97F, 0.16F };
			add( std::move( p ) );
		}

		// ---- Light ----

		add( make( "github-light", 0xffffff, 0x1f2328, 0x656d76, 0xcf222e, 0xbc4c00, 0x9a6700, 0x1a7f37, 0x1b7c83, 0x0969da, 0x8250df, 0xbf3989 ) );
		add( make( "catppuccin-latte", 0xeff1f5, 0x4c4f69, 0x8c8fa1, 0xd20f39, 0xfe640b, 0xdf8e1d, 0x40a02b, 0x179299, 0x1e66f5, 0x8839ef, 0xea76cb ) );
		add( make( "solarized-light", 0xfdf6e3, 0x586e75, 0x93a1a1, 0xdc322f, 0xcb4b16, 0xb58900, 0x859900, 0x2aa198, 0x268bd2, 0x6c71c4, 0xd33682 ) );

		// Paper: a printed page, warm white with dark inks (the light side of academic)
		{
			Palette p = make( "paper", 0xfaf6ec, 0x2b2b2b, 0x8a8478, 0xa33b3b, 0xb26a1e, 0x9a7d1c, 0x4f7a3a, 0x2f7d80, 0x2f5d8c, 0x6b4c8c, 0x9c3f6e );
			p.code.set( "keyword", { p.red, Weight::BOLD } );
			p.code.set( "property", { p.purple } );
			p.code.set( "operator", { p.text } );
			p.code.set( "delimiter", { p.text } );
			add( std::move( p ) );
		}
		return all;
	}();
	return palettes;
}

}

const Palette *Palette::find( std::string_view name )
{
	const auto found = registry().find( name );
	return found == registry().end() ? nullptr : &found->second;
}

const Palette &Palette::get( std::string_view name )
{
	if ( const Palette *found = find( name ) )
	{
		return *found;
	}
	std::string message = "unknown palette '" + std::string( name ) + "'; the palettes are:";
	for ( const std::string &known: names() )
	{
		message += " " + known;
	}
	throw std::invalid_argument( message );
}

std::vector<std::string> Palette::names()
{
	std::vector<std::string> out;
	for ( const auto &[name, palette]: registry() )
	{
		out.push_back( name );
	}
	return out;
}

void Palette::add( Palette palette )
{
	const std::string name = palette.name;
	registry().insert_or_assign( name, std::move( palette ) );
}

Palette Palette::house( std::string_view name )
{
	return get( name ).with_background( Color{ 0.F, 0.F, 0.F, 1.F } );
}
