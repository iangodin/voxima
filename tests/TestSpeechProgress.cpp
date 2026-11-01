#include <catch2/catch_test_macros.hpp>

#include <tts/TTSClient.h>

#include <string>

using tts::describe_speech;
using tts::short_line;
using tts::SpeechStats;

TEST_CASE( "A line is cut short at a character", "[tts]" )
{
	CHECK( short_line( "short" ) == "short" );
	CHECK( short_line( "abcdefgh", 8 ) == "abcdefgh" );
	CHECK( short_line( "abcdefghi", 8 ) == "abcdefgh…" );
	// "é" is two bytes, and byte 2 is in the middle of it
	CHECK( short_line( "aéb cd", 2 ) == "a…" );
}

TEST_CASE( "What the speech came to is told", "[tts]" )
{
	CHECK( describe_speech( {} ).empty() );

	SpeechStats stats;
	stats.lines_cached = 1;
	stats.seconds_cached = 3.0;
	CHECK( describe_speech( stats ) == "1 line, 3.0 s of speech: 1 from the cache, 0 made" );

	stats.lines_cached = 390;
	stats.seconds_cached = 1500.0;
	stats.lines_made = 22;
	stats.seconds_made = 180.0;
	stats.making_time = 366.0;
	CHECK( describe_speech( stats ) == "412 lines, 28.0 min of speech: 390 from the cache, 22 made in 6.1 min" );
}
