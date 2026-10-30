#include <catch2/catch_approx.hpp>
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

TEST_CASE( "progress runs from 0 to 1 across a span", "[audio]" )
{
	CHECK( progress( 5.0, 10.0, 20.0 ) == 0.F );
	CHECK( progress( 10.0, 10.0, 20.0 ) == 0.F );
	CHECK( progress( 15.0, 10.0, 20.0 ) == 0.5F );
	CHECK( progress( 20.0, 10.0, 20.0 ) == 1.F );
	CHECK( progress( 99.0, 10.0, 20.0 ) == 1.F );
}

TEST_CASE( "progress of a span with no length is 1 once reached", "[audio]" )
{
	CHECK( progress( 1.0, 2.0, 2.0 ) == 0.F );
	CHECK( progress( 2.0, 2.0, 2.0 ) == 1.F );
	CHECK( progress( 3.0, 2.0, 2.0 ) == 1.F );
}

namespace
{
// "one two three" said in 6 seconds, two seconds a word
std::vector<TimedWord> one_two_three()
{
	return {
		{ "one", 0, 0, 3, 0.0, 2.0 },
		{ "two", 0, 4, 7, 2.0, 4.0 },
		{ "three", 0, 8, 13, 4.0, 6.0 } };
}
}

TEST_CASE( "time_at_offset is when the speech gets to a cue point", "[audio]" )
{
	const auto words = one_two_three();

	SECTION( "before a word, its start" )
	{
		CHECK( time_at_offset( words, 0, -1.0 ) == 0.0 );
		CHECK( time_at_offset( words, 4, -1.0 ) == 2.0 );
		CHECK( time_at_offset( words, 8, -1.0 ) == 4.0 );
	}

	SECTION( "in the space after a word, the start of the next: when the speech picks up again" )
	{
		CHECK( time_at_offset( words, 3, -1.0 ) == 2.0 );
		CHECK( time_at_offset( words, 7, -1.0 ) == 4.0 );
	}

	SECTION( "inside a word, how far through it in letters" )
	{
		// "three" is 8..13 and takes 4.0 to 6.0; two letters in is 2/5 of the way
		CHECK( time_at_offset( words, 10, -1.0 ) == Catch::Approx( 4.0 + 2.0 * 2.0 / 5.0 ) );
	}

	SECTION( "after the last word, when it ends" )
	{
		CHECK( time_at_offset( words, 13, -1.0 ) == 6.0 );
		CHECK( time_at_offset( words, 99, -1.0 ) == 6.0 );
	}

	SECTION( "a line with no words uses the fallback" )
	{
		CHECK( time_at_offset( {}, 0, 3.5 ) == 3.5 );
	}

	SECTION( "counts letters, not bytes" )
	{
		// "café" is 5 bytes, 4 letters; a cue after "ca" (2 bytes in) is half way
		const std::vector<TimedWord> accented{ { "caf\xC3\xA9", 0, 0, 5, 0.0, 4.0 } };
		CHECK( time_at_offset( accented, 2, -1.0 ) == Catch::Approx( 2.0 ) );
	}
}

TEST_CASE( "marker_mismatches names what differs from the main language", "[audio]" )
{
	SECTION( "all the same is fine" )
	{
		CHECK( marker_mismatches( { { "en", { "a", "b" } }, { "fr", { "a", "b" } } }, "en" ).empty() );
		CHECK( marker_mismatches( { { "en", {} }, { "fr", {} } }, "en" ).empty() );
		CHECK( marker_mismatches( { { "en", { "a" } } }, "en" ).empty() );
	}

	SECTION( "a marker a translator forgot" )
	{
		const auto problems = marker_mismatches( { { "en", { "a", "b", "c" } }, { "fr", { "a", "c" } } }, "en" );
		REQUIRE( problems.size() == 1 );
		CHECK( problems[0].find( "fr" ) != std::string::npos );
		CHECK( problems[0].find( "missing: b" ) != std::string::npos );
	}

	SECTION( "an extra one" )
	{
		const auto problems = marker_mismatches( { { "en", { "a" } }, { "de", { "a", "z" } } }, "en" );
		REQUIRE( problems.size() == 1 );
		CHECK( problems[0].find( "not in en: z" ) != std::string::npos );
	}

	SECTION( "the same markers in another order" )
	{
		const auto problems = marker_mismatches( { { "en", { "a", "b" } }, { "fr", { "b", "a" } } }, "en" );
		REQUIRE( problems.size() == 1 );
		CHECK( problems[0].find( "different order" ) != std::string::npos );
	}

	SECTION( "one line for each language that differs" )
	{
		const auto problems = marker_mismatches( { { "en", { "a" } }, { "fr", {} }, { "de", {} }, { "es", { "a" } } }, "en" );
		CHECK( problems.size() == 2 );
	}

	SECTION( "no main language, nothing to compare with" )
	{
		CHECK( marker_mismatches( { { "fr", { "a" } } }, "en" ).empty() );
	}
}
