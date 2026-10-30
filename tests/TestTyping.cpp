#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <code/Typing.h>
#include <core/Theme.h>
#include <themes/NewComputerModern.h>
#include <visuals/Glyph.h>

#include <algorithm>

using Catch::Approx;

namespace
{
constexpr std::string_view SOURCE = "int main()\n{\n    return 0;\n}";

size_t at( std::string_view part )
{
	return SOURCE.find( part );
}
}

TEST_CASE( "typing_schedule gives every character a time, in order, starting at 0", "[typing]" )
{
	const auto times = typing_schedule( SOURCE );
	REQUIRE( times.size() == SOURCE.size() );
	CHECK( times.front() == 0.0 );
	for ( size_t i = 1; i < times.size(); ++i )
	{
		CHECK( times[i] >= times[i - 1] );
	}
}

TEST_CASE( "typing_schedule goes at the speed asked for, on average", "[typing]" )
{
	TypingRhythm rhythm;
	rhythm.chars_per_second = 10.0;
	const auto times = typing_schedule( SOURCE, rhythm );
	// 4 spaces of indentation are not typed
	const size_t keys = SOURCE.size() - 4;
	CHECK( times.back() == Approx( static_cast<double>( keys - 1 ) / 10.0 ) );
}

TEST_CASE( "typing_schedule is the same every time for a seed, and another for another seed", "[typing]" )
{
	TypingRhythm a;
	TypingRhythm b;
	b.seed = 2;
	CHECK( typing_schedule( SOURCE, a ) == typing_schedule( SOURCE, a ) );
	CHECK( typing_schedule( SOURCE, a ) != typing_schedule( SOURCE, b ) );
}

TEST_CASE( "typing_schedule pauses at the end of a line, and indents at once", "[typing]" )
{
	const auto times = typing_schedule( SOURCE );
	auto gap_before = [&]( size_t i ) { return times[i] - times[i - 1]; };

	// Within "main", quicker than the pause before the first thing on the next line
	const double in_word = gap_before( at( "main" ) + 1 );
	const double new_line = gap_before( at( "{" ) );
	CHECK( new_line > 2.0 * in_word );

	// The indentation comes with the newline before it, and "return" after a pause
	const size_t indent = at( "    return" );
	for ( size_t i = indent; i < indent + 4; ++i )
	{
		CHECK( times[i] == times[indent - 1] );
	}
	CHECK( times[at( "return" )] > times[indent] );
}

TEST_CASE( "typing_schedule without the human rhythm types evenly", "[typing]" )
{
	TypingRhythm even;
	even.human = false;
	const auto times = typing_schedule( "abc de", even );
	for ( size_t i = 1; i < times.size(); ++i )
	{
		CHECK( times[i] - times[i - 1] == Approx( 1.0 / even.chars_per_second ) );
	}
}

TEST_CASE( "typing_schedule gives the bytes of a character one time", "[typing]" )
{
	const auto times = typing_schedule( "aéb" ); // é is two bytes
	REQUIRE( times.size() == 4 );
	CHECK( times[2] == times[1] );
	CHECK( times[3] > times[2] );
}

TEST_CASE( "Typewriter shows what has been typed, fading each character in", "[typing]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::CPP, SOURCE );
	REQUIRE( listing->glyphs.size() == SOURCE.size() - 3 ); // every character but the three newlines has a glyph

	TypingRhythm rhythm;
	rhythm.fade_seconds = 0.1;
	Typewriter typer( *listing, rhythm );
	const auto times = typing_schedule( listing->text, rhythm );
	auto alpha_of = [&]( std::string_view part ) -> float
	{
		for ( const CodeGlyph &g: listing->glyphs )
		{
			if ( g.offset == at( part ) )
				return g.glyph->color.a;
		}
		FAIL( "no glyph for " << part );
		return -1.F;
	};

	// Nothing to start with
	for ( const CodeGlyph &g: listing->glyphs )
	{
		CHECK( g.glyph->color.a == 0.F );
	}

	// Just after "return" is typed: its r half faded in, "{" in full, "0" not yet
	const double r = times[at( "return" )];
	typer.set_time( r + 0.05 );
	CHECK( alpha_of( "return" ) == Approx( 0.5F ) );
	CHECK( alpha_of( "{" ) == 1.F );
	CHECK( alpha_of( "0;" ) == 0.F );

	// All of it at the end, as it was
	typer.set( 1.F );
	for ( const CodeGlyph &g: listing->glyphs )
	{
		CHECK( g.glyph->color.a == 1.F );
	}
	CHECK( typer.duration() == Approx( times.back() + 0.1 ) );

	// and back again (a preview can play backwards)
	typer.set( 0.F );
	CHECK( alpha_of( "int" ) == 0.F );
}

TEST_CASE( "make_code's glyphs are in the order of the source, with their lines and columns", "[typing]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::CPP, SOURCE );
	for ( size_t i = 1; i < listing->glyphs.size(); ++i )
	{
		CHECK( listing->glyphs[i].offset > listing->glyphs[i - 1].offset );
	}
	const auto r = std::find_if( listing->glyphs.begin(), listing->glyphs.end(), []( const CodeGlyph &g ) { return g.offset == at( "return" ); } );
	REQUIRE( r != listing->glyphs.end() );
	CHECK( r->line == 2 );
	CHECK( r->column == 4 );
}
