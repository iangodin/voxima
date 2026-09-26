#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <code/CodePalette.h>
#include <code/CreateCode.h>
#include <core/Theme.h>
#include <themes/NewComputerModern.h>
#include <visuals/Glyph.h>
#include <visuals/TextLine.h>

#include <algorithm>
#include <variant>

using Catch::Approx;

namespace
{
// The glyphs of a line that draw something, left to right
std::vector<const Glyph *> inked( const Visual &line )
{
	std::vector<const Glyph *> out;
	auto walk = [&]( auto &self, const Visual &v ) -> void
	{
		if ( const auto *g = dynamic_cast<const Glyph *>( &v ) )
		{
			// (a space has nothing but a Close)
			const bool draws = std::any_of( g->commands.begin(), g->commands.end(), []( const PathCommand &c ) { return !std::holds_alternative<MoveTo>( c ) && !std::holds_alternative<Close>( c ); } );
			if ( draws )
			{
				out.push_back( g );
			}
		}
		else if ( const auto *group = dynamic_cast<const Group *>( &v ) )
		{
			for ( const auto &child: *group )
			{
				self( self, *child );
			}
		}
	};
	walk( walk, line );
	std::sort( out.begin(), out.end(), []( const Glyph *a, const Glyph *b ) { return a->position.x() < b->position.x(); } );
	return out;
}
}

TEST_CASE( "prepare_code makes tabs spaces to the tab stops, and drops the edges' newlines", "[code]" )
{
	CHECK( prepare_code( "\tx", 4 ) == "    x" );
	CHECK( prepare_code( "ab\tx", 4 ) == "ab  x" );
	CHECK( prepare_code( "abcd\tx", 4 ) == "abcd    x" );
	CHECK( prepare_code( "a\n\tb", 2 ) == "a\n  b" ); // columns start again on each line
	CHECK( prepare_code( "é\tx", 4 ) == "é   x" ); // é is one column, two bytes
	CHECK( prepare_code( "a\r\nb\r\n", 4 ) == "a\nb" );
	CHECK( prepare_code( "\nint x;\n", 4 ) == "int x;" ); // as a raw string literal written on its own lines
	CHECK( prepare_code( "\n\nx\n\n", 4 ) == "\nx\n" ); // only one of each
}

TEST_CASE( "CodePalette falls back from a kind to the kind it is a kind of, then to plain", "[code]" )
{
	CodePalette palette;
	palette.plain = { { 1.F, 1.F, 1.F } };
	palette.set( "function", { { 0.F, 0.F, 1.F } } );
	palette.set( "function.special", { { 1.F, 0.F, 0.F } } );

	CHECK( palette.style_for( "function" ).color == Color{ 0.F, 0.F, 1.F } );
	CHECK( palette.style_for( "function.special" ).color == Color{ 1.F, 0.F, 0.F } );
	CHECK( palette.style_for( "function.method.call" ).color == Color{ 0.F, 0.F, 1.F } );
	CHECK( palette.style_for( "keyword" ).color == Color{ 1.F, 1.F, 1.F } );
}

TEST_CASE( "make_code lays out highlighted code, a line to a line, with the columns lined up", "[code]" )
{
	Theme theme = NewComputerModern::create_theme();
	const CodePalette &palette = theme.code_palette();

	auto code = make_code( theme, code::Language::CPP, R"cpp(
int main()
{
	return 0; // done
}
)cpp" );

	REQUIRE( code->size() == 4 );
	std::vector<std::vector<const Glyph *>> lines;
	for ( const auto &line: *code )
	{
		REQUIRE( dynamic_cast<const TextLine *>( line.get() ) );
		lines.push_back( inked( *line ) );
	}

	// Each part in its palette color
	CHECK( lines[0].front()->color == palette.style_for( "type" ).color );      // int
	CHECK( lines[0][3]->color == palette.style_for( "function" ).color );       // main
	CHECK( lines[1].front()->color == palette.plain.color );                    // {
	CHECK( lines[2].front()->color == palette.style_for( "keyword" ).color );   // return
	CHECK( lines[2][6]->color == palette.style_for( "number" ).color );         // 0
	CHECK( lines[2].back()->color == palette.style_for( "comment" ).color );    // // done

	// The tab is four spaces, so "return" starts in the column of "main" (after "int ")
	CHECK( lines[2].front()->position.x() == Approx( lines[0][3]->position.x() ).margin( 1e-4 ) );
	// and the lines go down
	CHECK( ( *code->begin() )->baseline > ( *( code->begin() + 1 ) )->baseline );
}
