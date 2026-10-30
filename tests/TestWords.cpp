#include <catch2/catch_test_macros.hpp>

#include <utils/Words.h>

#include <string>
#include <vector>

namespace
{
std::vector<std::string> words_of( std::string_view text )
{
	std::vector<std::string> out;
	for ( auto span: split_words( text ) )
	{
		out.emplace_back( text.substr( span.begin, span.end - span.begin ) );
	}
	return out;
}

using Words = std::vector<std::string>;
}

TEST_CASE( "split_words splits on whitespace and keeps punctuation attached", "[utils]" )
{
	CHECK( words_of( "Hello, world!" ) == Words{ "Hello,", "world!" } );
	CHECK( words_of( "  a \t b\nc  " ) == Words{ "a", "b", "c" } );
	CHECK( words_of( "" ).empty() );
	CHECK( words_of( " \n " ).empty() );
	CHECK( words_of( "3.14 e-mail" ) == Words{ "3.14", "e-mail" } );
}

TEST_CASE( "split_words reports byte offsets", "[utils]" )
{
	const std::string text = "  zażółć ab";
	auto spans = split_words( text );
	REQUIRE( spans.size() == 2 );
	CHECK( spans[0] == WordSpan{ 2, 12 } );
	CHECK( spans[1] == WordSpan{ 13, 15 } );
}

TEST_CASE( "split_words drops tokens with nothing to pronounce", "[utils]" )
{
	CHECK( words_of( "wait — what" ) == Words{ "wait", "what" } );
	CHECK( words_of( "a - b" ) == Words{ "a", "b" } );
	CHECK( words_of( "\"...\"" ).empty() );
	// spoken symbols are words
	CHECK( words_of( "this & that" ) == Words{ "this", "&", "that" } );
	CHECK( words_of( "50 %" ) == Words{ "50", "%" } );
}

TEST_CASE( "split_words treats no-break spaces as part of the word", "[utils]" )
{
	CHECK( words_of( "« mot » seul" ) == Words{ "« mot »", "seul" } );
}

TEST_CASE( "split_words gives each ideograph or kana its own word", "[utils]" )
{
	CHECK( words_of( "日本語" ) == Words{ "日", "本", "語" } );
	// prolonged sound mark and voiced mark attach backward
	CHECK( words_of( "コーヒー" ) == Words{ "コー", "ヒー" } );
	// stops attach backward, opening quotes forward
	CHECK( words_of( "「日本。」" ) == Words{ "「日", "本。」" } );
	// mixed with spaced text
	CHECK( words_of( "iPhoneを買う" ) == Words{ "iPhone", "を", "買", "う" } );
}

TEST_CASE( "split_words survives invalid UTF-8", "[utils]" )
{
	CHECK( words_of( "a\xFF" "b c" ) == Words{ "a\xFF" "b", "c" } );
}

TEST_CASE( "codepoint_count counts code points, not bytes", "[utils]" )
{
	CHECK( codepoint_count( "" ) == 0 );
	CHECK( codepoint_count( "abc" ) == 3 );
	CHECK( codepoint_count( "zażółć" ) == 6 );
}
