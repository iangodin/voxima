#include <catch2/catch_test_macros.hpp>

#include <utils/AltText.h>

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
