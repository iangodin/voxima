#include <catch2/catch_test_macros.hpp>

#include <core/Script.h>
#include <utils/AltText.h>
#include <utils/Markers.h>

#include <filesystem>
#include <fstream>

#include <string>

TEST_CASE( "text without a tag is left alone", "[alttext]" )
{
	const auto a = parse_alt_text( "We built C++ in a weekend." );
	CHECK( a.written == "We built C++ in a weekend." );
	CHECK( a.spoken == "We built C++ in a weekend." );

	CHECK( parse_alt_text( "" ).written.empty() );
	CHECK( parse_alt_text( "" ).spoken.empty() );
}

TEST_CASE( "<written|spoken> shows one thing and says another", "[alttext]" )
{
	const auto a = parse_alt_text( "We built <C++|see plus plus> in a weekend." );
	CHECK( a.written == "We built C++ in a weekend." );
	CHECK( a.spoken == "We built see plus plus in a weekend." );
}

TEST_CASE( "<written> alone is shown but not spoken", "[alttext]" )
{
	const auto a = parse_alt_text( "Try <voxima_demo --list-scenes> on the command line." );
	CHECK( a.written == "Try voxima_demo --list-scenes on the command line." );
	CHECK( a.spoken == "Try on the command line." );
}

TEST_CASE( "an unspoken tag at either end, or the whole line, leaves no stray space", "[alttext]" )
{
	CHECK( parse_alt_text( "<Voxima> is the title" ).spoken == "is the title" );
	CHECK( parse_alt_text( "the title is <Voxima>" ).spoken == "the title is" );
	CHECK( parse_alt_text( "<Voxima>" ).spoken.empty() );
	CHECK( parse_alt_text( "<a> <b> word <c>" ).spoken == "word" );
}

TEST_CASE( "several tags in one line", "[alttext]" )
{
	const auto a = parse_alt_text( "<C++|see plus plus> and <SQL|sequel> are both old." );
	CHECK( a.written == "C++ and SQL are both old." );
	CHECK( a.spoken == "see plus plus and sequel are both old." );
}

TEST_CASE( "a doubled delimiter is the literal character", "[alttext]" )
{
	CHECK( parse_alt_text( "a << b" ).written == "a < b" );
	CHECK( parse_alt_text( "a << b" ).spoken == "a < b" );
	CHECK( parse_alt_text( "a >> b" ).written == "a > b" );
	CHECK( parse_alt_text( "a || b" ).written == "a | b" );

	const auto mixed = parse_alt_text( "<< then <A|B>" );
	CHECK( mixed.written == "< then A" );
	CHECK( mixed.spoken == "< then B" );
}

TEST_CASE( "a '<' with no matching '>' is ordinary text", "[alttext]" )
{
	CHECK( parse_alt_text( "a < b" ).written == "a < b" );
	CHECK( parse_alt_text( "a < b" ).spoken == "a < b" );
	CHECK( parse_alt_text( "open <never closed" ).written == "open <never closed" );
}

TEST_CASE( "written_text is the written side alone", "[alttext]" )
{
	CHECK( written_text( "We built <C++|see plus plus> in a weekend." ) == "We built C++ in a weekend." );
}

namespace
{
const Pronunciations WORDS{
	{ "HLG", "hybrid log gamma" },
	{ "OETF", "oh ee tee eff" },
	{ "cd/m²", "candelas per square meter" },
	{ "BT.2100", "bee tee twenty one hundred" },
	{ "C++", "see plus plus" },
	{ "BT", "bee tee" },
};
}

TEST_CASE( "Words are said as the list says, and shown as they are", "[pronounce]" )
{
	const auto a = parse_alt_text( "The OETF is 100 cd/m² in HLG.", WORDS );
	CHECK( a.written == "The OETF is 100 cd/m² in HLG." );
	CHECK( a.spoken == "The oh ee tee eff is 100 candelas per square meter in hybrid log gamma." );
}

TEST_CASE( "Words are found whole", "[pronounce]" )
{
	CHECK( apply_pronunciations( "OETFs and xOETF and OETF2", WORDS ) == "OETFs and xOETF and OETF2" );
	CHECK( apply_pronunciations( "(OETF), OETF.", WORDS ) == "(oh ee tee eff), oh ee tee eff." );
	CHECK( apply_pronunciations( "oetf", WORDS ) == "oetf" ); // case matters
	CHECK( apply_pronunciations( "OETF", WORDS ) == "oh ee tee eff" );
	CHECK( apply_pronunciations( "cd/m²s", WORDS ) == "cd/m²s" );
	CHECK( apply_pronunciations( "éHLG", WORDS ) == "éHLG" );
	CHECK( apply_pronunciations( "C++ and xC++", WORDS ) == "see plus plus and xC++" );
}

TEST_CASE( "The longest word that fits is used", "[pronounce]" )
{
	CHECK( apply_pronunciations( "BT.2100 and BT", WORDS ) == "bee tee twenty one hundred and bee tee" );
}

TEST_CASE( "What a word becomes is not looked at again", "[pronounce]" )
{
	const Pronunciations list{ { "a", "b" }, { "b", "c" } };
	CHECK( apply_pronunciations( "a b", list ) == "b c" );
}

TEST_CASE( "A spoken form in the text is as it is, and markers stay markers", "[pronounce]" )
{
	const auto a = parse_alt_text( "HLG <HLG|aitch ell gee HLG> HLG", WORDS );
	CHECK( a.spoken == "hybrid log gamma aitch ell gee HLG hybrid log gamma" );
	CHECK( a.written == "HLG HLG HLG" );

	const auto marked = parse_alt_text( "Say {HLG}HLG{OETF} now", WORDS );
	const MarkedText text = parse_markers( marked.spoken );
	CHECK( text.text == "Say hybrid log gamma now" );
	REQUIRE( text.markers.size() == 2 );
	CHECK( text.markers[0].name == "HLG" );
	CHECK( text.markers[1].name == "OETF" );

	CHECK( apply_pronunciations( "{{HLG}}", WORDS ) == "{{hybrid log gamma}}" );
	CHECK( apply_pronunciations( "{ unclosed HLG", WORDS ) == "{ unclosed hybrid log gamma" );
	CHECK( apply_pronunciations( "{a, HLG}", WORDS ) == "{a, hybrid log gamma}" ); // not a marker
}

TEST_CASE( "No list changes nothing", "[pronounce]" )
{
	CHECK( parse_alt_text( "The OETF" ).spoken == "The OETF" );
	CHECK( apply_pronunciations( "", WORDS ).empty() );
}

TEST_CASE( "A script's pronounce section is read for each language", "[pronounce]" )
{
	const auto directory = std::filesystem::temp_directory_path() / "voxima-test-pronounce";
	std::filesystem::remove_all( directory );
	std::filesystem::create_directories( directory );
	const auto write = [&]( const std::string &name, const std::string &text ) { std::ofstream( directory / name ) << text; };
	write( "en.huml", "meta::\n  lang: \"en\"\n\npronounce::\n  \"cd/m²\": \"candelas\"\n  OETF: \"oh ee\"\n" );
	write( "en-GB.huml", "meta::\n  lang: \"en-GB\"\n  inherit: \"en\"\n\npronounce::\n  OETF: \"oh eee\"\n" );
	write( "fr.huml", "meta::\n  lang: \"fr\"\n" );
	write( "bad.huml", "meta::\n  lang: \"fr\"\n\npronounce::\n  OETF: 5\n" );
	write( "worse.huml", "meta::\n  lang: \"fr\"\n\npronounce: \"no\"\n" );

	Script script;
	script.set_directory( directory );
	REQUIRE( script.load( "en" ) );
	REQUIRE( script.load( "en-GB" ) );
	REQUIRE( script.load( "fr" ) );
	CHECK( apply_pronunciations( "100 cd/m² OETF", script.pronunciations( "en" ) ) == "100 candelas oh ee" );
	CHECK( apply_pronunciations( "100 cd/m² OETF", script.pronunciations( "en-GB" ) ) == "100 candelas oh eee" ); // merged
	CHECK( script.pronunciations( "fr" ).empty() );
	CHECK( script.pronunciations( "not loaded" ).empty() );
	CHECK_THROWS_AS( script.load( "bad" ), std::runtime_error );
	CHECK_THROWS_AS( script.load( "worse" ), std::runtime_error );
	std::filesystem::remove_all( directory );
}
