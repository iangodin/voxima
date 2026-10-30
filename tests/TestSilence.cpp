#include <catch2/catch_test_macros.hpp>
#include <audio/Silence.h>

#include <algorithm>

TEST_CASE( "make_silence produces a zeroed buffer of the requested size", "[audio]" )
{
	AudioBuffer buffer = make_silence( 100, 48000, 2 );

	CHECK( buffer.frames() == 100 );
	CHECK( buffer.rate() == 48000 );
	CHECK( buffer.channels() == 2 );
	CHECK( std::all_of( buffer.begin(), buffer.end(), []( float s ) { return s == 0.F; } ) );
}

TEST_CASE( "make_silence from a duration rounds to the nearest frame", "[audio]" )
{
	AudioBuffer buffer = make_silence_for_duration( 0.5, 48000, 1 );

	CHECK( buffer.frames() == 24000 );
	CHECK( buffer.rate() == 48000 );
	CHECK( buffer.channels() == 1 );
}
