#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <audio/Retime.h>

#include <algorithm>

using Catch::Approx;

namespace
{
constexpr size_t RATE = 48000;

// A second-long stretch of one value: 0.1, 0.2, 0.3 for the segments, so where each one went is easy to see
AudioBuffer three_segments( size_t channels = 1 )
{
	AudioBuffer natural( RATE * 4, RATE, channels ); // 0-1 s, 1-3 s, 3-4 s
	for ( size_t f = 0; f < natural.frames(); ++f )
	{
		const float value = f < RATE ? 0.1F : ( f < 3 * RATE ? 0.2F : 0.3F );
		for ( size_t c = 0; c < channels; ++c )
		{
			natural.frame( f )[c] = value;
		}
	}
	return natural;
}

Timeline cuts_at_1_and_3()
{
	Timeline t;
	t.cuts = { { "\nstart", 0.0 }, { "a", 1.0 }, { "b", 3.0 } };
	t.end = 4.0;
	return t;
}

float at( const AudioBuffer &b, double seconds, size_t channel = 0 )
{
	return b.frame( static_cast<size_t>( seconds * static_cast<double>( RATE ) ) )[channel];
}
}

TEST_CASE( "retime_audio puts each segment where the shared timeline says, with silence between", "[retime]" )
{
	// each segment is given 0.5 s more than it needs: 1.5, 2.5 and 1.5 s long, starting at 0, 1.5 and 4.0
	const std::vector<double> starts{ 0.0, 1.5, 4.0, 5.5 };
	const AudioBuffer out = retime_audio( three_segments(), cuts_at_1_and_3(), starts );

	CHECK( out.frames() == RATE * 11 / 2 );
	CHECK( out.rate() == RATE );

	// the first segment (0.1), then silence
	CHECK( at( out, 0.5 ) == Approx( 0.1F ) );
	CHECK( at( out, 0.99 ) == Approx( 0.1F ).margin( 0.11 ) ); // (faded at its end, next to the silence)
	CHECK( at( out, 1.2 ) == 0.F );
	CHECK( at( out, 1.49 ) == 0.F );

	// the second (0.2), from 1.5 to 3.5, then silence
	CHECK( at( out, 2.0 ) == Approx( 0.2F ) );
	CHECK( at( out, 3.0 ) == Approx( 0.2F ) );
	CHECK( at( out, 3.7 ) == 0.F );
	CHECK( at( out, 3.99 ) == 0.F );

	// the third (0.3), from 4.0 to 5.0, then silence
	CHECK( at( out, 4.5 ) == Approx( 0.3F ) );
	CHECK( at( out, 5.2 ) == 0.F );
}

TEST_CASE( "retime_audio fades only where silence is added next to speech", "[retime]" )
{
	const std::vector<double> starts{ 0.0, 1.5, 4.0, 5.5 };
	const AudioBuffer out = retime_audio( three_segments(), cuts_at_1_and_3(), starts );
	const size_t fade = RATE * 2 / 1000;

	SECTION( "not at the very start of the audio" )
	{
		CHECK( out.frame( 0 )[0] == Approx( 0.1F ) );
	}

	SECTION( "at the end of a segment that is followed by silence, down to nothing" )
	{
		const size_t last = RATE - 1; // the last frame of the first segment
		CHECK( out.frame( last )[0] == Approx( 0.F ).margin( 0.01 ) );
		CHECK( out.frame( last - fade )[0] > 0.09F );
		// and it ramps: each frame louder than the one after it
		CHECK( out.frame( last - 10 )[0] > out.frame( last - 5 )[0] );
	}

	SECTION( "at the start of a segment that comes after silence, up from nothing" )
	{
		const size_t first = RATE * 3 / 2; // where the second segment goes
		CHECK( out.frame( first )[0] == Approx( 0.F ).margin( 0.01 ) );
		CHECK( out.frame( first + fade + 1 )[0] == Approx( 0.2F ) );
	}
}

TEST_CASE( "segments that follow each other with nothing added are copied straight through", "[retime]" )
{
	// the natural lengths: nothing to add anywhere
	const std::vector<double> starts{ 0.0, 1.0, 3.0, 4.0 };
	const AudioBuffer natural = three_segments();
	const AudioBuffer out = retime_audio( natural, cuts_at_1_and_3(), starts );

	REQUIRE( out.frames() == natural.frames() );
	bool same = true;
	for ( size_t f = 0; f < natural.frames(); ++f )
	{
		same = same && out.frame( f )[0] == natural.frame( f )[0];
	}
	CHECK( same ); // no fade dips at the joins
}

TEST_CASE( "retime_audio can also move a segment later without padding the others", "[retime]" )
{
	// only the middle segment is longer: 0.5 s of silence after it
	const std::vector<double> starts{ 0.0, 1.0, 3.5, 4.5 };
	const AudioBuffer out = retime_audio( three_segments(), cuts_at_1_and_3(), starts );

	CHECK( at( out, 0.5 ) == Approx( 0.1F ) );
	CHECK( at( out, 0.9999 ) == Approx( 0.1F ) ); // straight into the second: no fade
	CHECK( at( out, 1.0001 ) == Approx( 0.2F ) );
	CHECK( at( out, 3.2 ) == 0.F );
	CHECK( at( out, 4.0 ) == Approx( 0.3F ) );
}

TEST_CASE( "retime_audio keeps every channel", "[retime]" )
{
	const std::vector<double> starts{ 0.0, 1.5, 4.0, 5.5 };
	const AudioBuffer out = retime_audio( three_segments( 2 ), cuts_at_1_and_3(), starts );

	REQUIRE( out.channels() == 2 );
	CHECK( at( out, 2.0, 0 ) == Approx( 0.2F ) );
	CHECK( at( out, 2.0, 1 ) == Approx( 0.2F ) );
	CHECK( at( out, 3.7, 1 ) == 0.F );
}

TEST_CASE( "retime_audio copes with cuts past the end of the audio", "[retime]" )
{
	Timeline t = cuts_at_1_and_3();
	t.end = 10.0; // longer than the audio really is
	const std::vector<double> starts{ 0.0, 1.5, 4.0, 12.0 };
	const AudioBuffer out = retime_audio( three_segments(), t, starts );

	CHECK( out.frames() == RATE * 12 );
	CHECK( at( out, 4.5 ) == Approx( 0.3F ) );
	CHECK( at( out, 8.0 ) == 0.F ); // nothing to copy, so silence
}
