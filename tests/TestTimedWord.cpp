#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <audio/TimedWord.h>

using Catch::Matchers::WithinAbs;

TEST_CASE( "distribute_words spreads words proportionally to their length", "[audio]" )
{
	auto words = distribute_words( "aa bbbbbb", 2, 10.0, 4.0 );

	REQUIRE( words.size() == 2 );
	CHECK( words[0].word == "aa" );
	CHECK( words[0].line == 2 );
	CHECK_THAT( words[0].start_seconds, WithinAbs( 10.0, 1e-9 ) );
	CHECK_THAT( words[0].end_seconds, WithinAbs( 11.0, 1e-9 ) );
	CHECK_THAT( words[1].start_seconds, WithinAbs( 11.0, 1e-9 ) );
	CHECK_THAT( words[1].end_seconds, WithinAbs( 14.0, 1e-9 ) );
}

TEST_CASE( "distribute_words counts UTF-8 code points, not bytes", "[audio]" )
{
	// "zażółć" is 6 code points (10 bytes); "ab" is 2.
	auto words = distribute_words( "zażółć ab", 0, 0.0, 8.0 );

	REQUIRE( words.size() == 2 );
	CHECK_THAT( words[0].end_seconds, WithinAbs( 6.0, 1e-9 ) );
}

TEST_CASE( "distribute_words ignores extra whitespace and empty text", "[audio]" )
{
	CHECK( distribute_words( "  \n ", 0, 0.0, 1.0 ).empty() );
	CHECK( distribute_words( "", 0, 0.0, 1.0 ).empty() );
	CHECK( distribute_words( "  a \t b ", 0, 0.0, 1.0 ).size() == 2 );
}

TEST_CASE( "distribute_words records byte offsets and uses the shared splitter", "[audio]" )
{
	auto words = distribute_words( "Hi, — there", 0, 0.0, 2.0 );

	REQUIRE( words.size() == 2 );
	CHECK( words[0].word == "Hi," );
	CHECK( words[0].begin == 0 );
	CHECK( words[0].end == 3 );
	CHECK( words[1].word == "there" );
}

namespace
{
struct Aligned
{
	double start_seconds;
	double end_seconds;
};
}

TEST_CASE( "match_words pairs aligner times with the text's words", "[audio]" )
{
	std::vector<Aligned> aligned{ { 0.1, 0.4 }, { 0.5, 0.9 } };
	auto words = match_words( "Hello world", 1, 10.0, aligned );

	REQUIRE( words.size() == 2 );
	CHECK( words[1].word == "world" );
	CHECK( words[1].line == 1 );
	CHECK_THAT( words[0].start_seconds, WithinAbs( 10.1, 1e-9 ) );
	CHECK_THAT( words[1].end_seconds, WithinAbs( 10.9, 1e-9 ) );
}

TEST_CASE( "match_words gives up when the word counts disagree", "[audio]" )
{
	std::vector<Aligned> aligned{ { 0.0, 1.0 } };
	CHECK( match_words( "two words", 0, 0.0, aligned ).empty() );
}
