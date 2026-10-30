#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <fonts/MathAlphabet.h>
#include <fonts/MathFont.h>
#include <themes/NewComputerModern.h>
#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
#include <visuals/Glyph.h>

TEST_CASE( "math alphabet: Latin letters", "[fonts]" )
{
	CHECK( map_math_char( U'x', MathAlphabet::ITALIC ) == 0x1D465 );
	CHECK( map_math_char( U'A', MathAlphabet::ITALIC ) == 0x1D434 );
	CHECK( map_math_char( U'z', MathAlphabet::ITALIC ) == 0x1D467 );
	CHECK( map_math_char( U'a', MathAlphabet::BOLD ) == 0x1D41A );
	CHECK( map_math_char( U'Z', MathAlphabet::BOLD_ITALIC ) == 0x1D481 );
	CHECK( map_math_char( U'a', MathAlphabet::SANS ) == 0x1D5BA );
	CHECK( map_math_char( U'a', MathAlphabet::MONO ) == 0x1D68A );
}

TEST_CASE( "math alphabet: letters that live in older blocks", "[fonts]" )
{
	CHECK( map_math_char( U'h', MathAlphabet::ITALIC ) == 0x210E );
	CHECK( map_math_char( U'B', MathAlphabet::SCRIPT ) == 0x212C );
	CHECK( map_math_char( U'e', MathAlphabet::SCRIPT ) == 0x212F );
	CHECK( map_math_char( U'C', MathAlphabet::FRAKTUR ) == 0x212D );
	CHECK( map_math_char( U'R', MathAlphabet::DOUBLE_STRUCK ) == 0x211D );
	CHECK( map_math_char( U'N', MathAlphabet::DOUBLE_STRUCK ) == 0x2115 );
	// neighbours of a hole still map into the block
	CHECK( map_math_char( U'g', MathAlphabet::ITALIC ) == 0x1D454 );
	CHECK( map_math_char( U'i', MathAlphabet::ITALIC ) == 0x1D456 );
	CHECK( map_math_char( U'A', MathAlphabet::SCRIPT ) == 0x1D49C );
}

TEST_CASE( "math alphabet: digits and Greek", "[fonts]" )
{
	// italic digits and capital Greek stay upright, as in TeX
	CHECK( map_math_char( U'7', MathAlphabet::ITALIC ) == U'7' );
	CHECK( map_math_char( 0x3A3, MathAlphabet::ITALIC ) == 0x3A3 );
	CHECK( map_math_char( 0x3B1, MathAlphabet::ITALIC ) == 0x1D6FC );
	CHECK( map_math_char( 0x3C9, MathAlphabet::ITALIC ) == 0x1D714 );
	CHECK( map_math_char( U'7', MathAlphabet::BOLD ) == 0x1D7D5 );
	CHECK( map_math_char( U'1', MathAlphabet::DOUBLE_STRUCK ) == 0x1D7D9 );
	CHECK( map_math_char( 0x391, MathAlphabet::BOLD ) == 0x1D6A8 );
	CHECK( map_math_char( 0x3A9, MathAlphabet::BOLD ) == 0x1D6C0 );
	CHECK( map_math_char( 0x3B1, MathAlphabet::BOLD ) == 0x1D6C2 );
}

TEST_CASE( "math alphabet: text", "[fonts]" )
{
	CHECK( map_math_text( "x+1", MathAlphabet::ITALIC ) == "\xF0\x9D\x91\xA5+1" );
	CHECK( map_math_text( "sin x", MathAlphabet::UPRIGHT ) == "sin x" );
	CHECK( map_math_text( "", MathAlphabet::ITALIC ).empty() );
	// other characters, including multi-byte ones, pass through
	CHECK( map_math_text( "\xE2\x88\x91", MathAlphabet::ITALIC ) == "\xE2\x88\x91" );
}

TEST_CASE( "math_alphabet: what a style asks for", "[fonts]" )
{
	SECTION( "the math intent defaults to math italic" )
	{
		CHECK( math_alphabet( FontStyle{ Intent::MATH } ) == MathAlphabet::ITALIC );
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.large() ) == MathAlphabet::ITALIC );
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.math_italic() ) == MathAlphabet::ITALIC );
	}

	SECTION( ".upright() opts out" )
	{
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.upright() ) == MathAlphabet::UPRIGHT );
	}

	SECTION( "bold, sans, mono, script, fraktur and double-struck pick their own alphabet upright" )
	{
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.bold() ) == MathAlphabet::BOLD );
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.sans() ) == MathAlphabet::SANS );
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.mono() ) == MathAlphabet::MONO );
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.script() ) == MathAlphabet::SCRIPT );
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.fraktur() ) == MathAlphabet::FRAKTUR );
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.double_struck() ) == MathAlphabet::DOUBLE_STRUCK );
	}

	SECTION( "explicit math italic combines with bold" )
	{
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.bold().math_italic() ) == MathAlphabet::BOLD_ITALIC );
	}

	SECTION( "true italic is a font request, not an alphabet" )
	{
		CHECK( math_alphabet( FontStyle{ Intent::MATH }.italic() ) == MathAlphabet::UPRIGHT );
	}

	SECTION( "other intents are untouched unless math italic is asked for" )
	{
		CHECK( math_alphabet( FontStyle{ Intent::PROSE } ) == MathAlphabet::UPRIGHT );
		CHECK( math_alphabet( FontStyle{ Intent::PROSE }.bold() ) == MathAlphabet::UPRIGHT );
		CHECK( math_alphabet( FontStyle{ Intent::PROSE }.math_italic() ) == MathAlphabet::ITALIC );
	}
}

namespace
{
// The glyph indices of a piece of text, in order
void collect_glyphs( const Visual &visual, std::vector<uint32_t> &out )
{
	if ( const auto *glyph = dynamic_cast<const Glyph *>( &visual ) )
	{
		out.push_back( glyph->index );
	}
	else if ( const auto *group = dynamic_cast<const Group *>( &visual ) )
	{
		for ( const auto &child: *group )
		{
			collect_glyphs( *child, out );
		}
	}
}

std::vector<uint32_t> glyphs_of( const Visual &visual )
{
	std::vector<uint32_t> out;
	collect_glyphs( visual, out );
	return out;
}
}

TEST_CASE( "make_text applies the math alphabet", "[fonts]" )
{
	auto theme = NewComputerModern::create_theme();
	MathFonts fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont font( fonts.roman() );

	SECTION( "letters in the math intent are math italic by default" )
	{
		auto glyphs = glyphs_of( *make_text( theme, "x", FontStyle{ Intent::MATH } ) );
		REQUIRE( glyphs.size() == 1 );
		CHECK( glyphs[0] == font.get_glyph_index( 0x1D465 ) );
	}

	SECTION( ".upright() gives the plain letter" )
	{
		auto glyphs = glyphs_of( *make_text( theme, "x", FontStyle{ Intent::MATH }.upright() ) );
		REQUIRE( glyphs.size() == 1 );
		CHECK( glyphs[0] == font.get_glyph_index( U'x' ) );
		CHECK( glyphs[0] != font.get_glyph_index( 0x1D465 ) );
	}

	SECTION( "digits are not remapped" )
	{
		auto italic = glyphs_of( *make_text( theme, "42", FontStyle{ Intent::MATH } ) );
		auto upright = glyphs_of( *make_text( theme, "42", FontStyle{ Intent::MATH }.upright() ) );
		CHECK( italic == upright );
	}

	SECTION( "other intents are not remapped" )
	{
		auto glyphs = glyphs_of( *make_text( theme, "x", FontStyle{ Intent::PROSE } ) );
		REQUIRE( glyphs.size() == 1 );
		CHECK( glyphs[0] != font.get_glyph_index( 0x1D465 ) );
	}

	SECTION( "one string can mix alphabets by range" )
	{
		StyledText text( "xy" );
		text.set_font_style( 0, text.size(), FontStyle{ Intent::MATH } );
		text.set_slant( 1, 2, Slant::UPRIGHT );

		auto glyphs = glyphs_of( *make_styled_text( theme, text ) );
		REQUIRE( glyphs.size() == 2 );
		CHECK( glyphs[0] == font.get_glyph_index( 0x1D465 ) );
		CHECK( glyphs[1] == font.get_glyph_index( U'y' ) );
	}
}
