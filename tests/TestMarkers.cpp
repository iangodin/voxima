#include <catch2/catch_test_macros.hpp>

#include <utils/Markers.h>

#include <string>
#include <vector>

namespace
{
// The text in front of a marker, to say where it is in words
std::string before( const MarkedText &m, size_t marker )
{
	return m.text.substr( 0, m.markers.at( marker ).offset );
}

std::vector<std::string> names( const MarkedText &m )
{
	std::vector<std::string> out;
	for ( const auto &marker: m.markers )
	{
		out.push_back( marker.name );
	}
	return out;
}
}

TEST_CASE( "text without markers is left alone", "[markers]" )
{
	const auto m = parse_markers( "These show various ease animations." );
	CHECK( m.text == "These show various ease animations." );
	CHECK( m.markers.empty() );

	CHECK( parse_markers( "" ).text.empty() );
	CHECK( parse_markers( "" ).markers.empty() );
}

TEST_CASE( "markers are taken out and remember where they were", "[markers]" )
{
	const auto m = parse_markers( "These show {a}various ease{b} animations." );

	CHECK( m.text == "These show various ease animations." );
	REQUIRE( m.markers.size() == 2 );
	CHECK( m.markers[0].name == "a" );
	CHECK( m.markers[1].name == "b" );
	CHECK( before( m, 0 ) == "These show " );      // just before "various"
	CHECK( before( m, 1 ) == "These show various ease" ); // just after "ease", before the space
}

TEST_CASE( "markers at the ends and next to each other", "[markers]" )
{
	SECTION( "at the start and the end" )
	{
		const auto m = parse_markers( "{start}Hello world{end}" );
		CHECK( m.text == "Hello world" );
		REQUIRE( m.markers.size() == 2 );
		CHECK( m.markers[0].offset == 0 );
		CHECK( m.markers[1].offset == m.text.size() );
	}

	SECTION( "side by side, and in a line with nothing else" )
	{
		const auto m = parse_markers( "{1}{2}{3}" );
		CHECK( m.text.empty() );
		CHECK( names( m ) == std::vector<std::string>{ "1", "2", "3" } );
		for ( const auto &marker: m.markers )
		{
			CHECK( marker.offset == 0 );
		}
	}

	SECTION( "the same name twice is kept twice, in order" )
	{
		const auto m = parse_markers( "a{x}b{x}c" );
		CHECK( names( m ) == std::vector<std::string>{ "x", "x" } );
		CHECK( m.markers[0].offset == 1 );
		CHECK( m.markers[1].offset == 2 );
	}
}

TEST_CASE( "names may hold letters, digits, underscores and hyphens", "[markers]" )
{
	const auto m = parse_markers( "{intro_1}a{Part-2}b{X9}" );
	CHECK( names( m ) == std::vector<std::string>{ "intro_1", "Part-2", "X9" } );
	CHECK( m.text == "ab" );
}

TEST_CASE( "a marker between two spaces leaves one", "[markers]" )
{
	const auto m = parse_markers( "ease {b} animations" );
	CHECK( m.text == "ease animations" );
	CHECK( before( m, 0 ) == "ease " );

	// but not where there was no space to lose
	CHECK( parse_markers( "ease{b} animations" ).text == "ease animations" );
	CHECK( parse_markers( "ease {b}animations" ).text == "ease animations" );
}

TEST_CASE( "braces that are not markers are text", "[markers]" )
{
	SECTION( "anything but a name inside" )
	{
		CHECK( parse_markers( "the set {a, b} of" ).text == "the set {a, b} of" );
		CHECK( parse_markers( "the set {a, b} of" ).markers.empty() );
		CHECK( parse_markers( "empty {} braces" ).text == "empty {} braces" );
		CHECK( parse_markers( "open { never closed" ).text == "open { never closed" );
		CHECK( parse_markers( "{a" ).markers.empty() );
		CHECK( parse_markers( "a}" ).text == "a}" );
	}

	SECTION( "a doubled brace is a literal one" )
	{
		const auto m = parse_markers( "{{a}}" );
		CHECK( m.text == "{a}" );
		CHECK( m.markers.empty() );

		const auto both = parse_markers( "{{x}} then {y}" );
		CHECK( both.text == "{x} then " + std::string() );
		CHECK( names( both ) == std::vector<std::string>{ "y" } );
	}
}

TEST_CASE( "offsets are bytes in the UTF-8 text without the markers", "[markers]" )
{
	const auto m = parse_markers( "caf\xC3\xA9 {a}au lait{b}" );
	CHECK( m.text == "caf\xC3\xA9 au lait" );
	REQUIRE( m.markers.size() == 2 );
	CHECK( m.markers[0].offset == 6 ); // "café " is 6 bytes
	CHECK( m.text.substr( m.markers[0].offset ) == "au lait" );
	CHECK( m.markers[1].offset == m.text.size() );

	const auto cjk = parse_markers( "\xE6\x97\xA5{a}\xE6\x9C\xAC" );
	CHECK( cjk.markers[0].offset == 3 );
}

TEST_CASE( "strip_markers gives the text alone", "[markers]" )
{
	CHECK( strip_markers( "Watch {c}light up{d} now" ) == "Watch light up now" );
	CHECK( strip_markers( "nothing" ) == "nothing" );
}
