#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <audio/Compress.h>
#include <audio/Retime.h>
#include <audio/Stretch.h>

#include <cmath>
#include <cstdlib>
#include <numbers>

using Catch::Approx;

namespace
{
constexpr size_t RATE = 8000;

// Stands in for ffmpeg: shortens by picking frames (the pitch would change, but the tests only look at lengths)
Stretcher fake_stretcher( int *calls = nullptr, double *last_tempo = nullptr )
{
	return [=]( const AudioBuffer &in, double tempo )
	{
		if ( calls ) ++*calls;
		if ( last_tempo ) *last_tempo = tempo;
		AudioBuffer out( static_cast<size_t>( std::llround( static_cast<double>( in.frames() ) / tempo ) ), in.rate(), in.channels() );
		for ( size_t f = 0; f < out.frames(); ++f )
		{
			const size_t src = std::min( in.frames() - 1, static_cast<size_t>( std::llround( static_cast<double>( f ) * tempo ) ) );
			for ( size_t c = 0; c < in.channels(); ++c )
			{
				out.frame( f )[c] = in.frame( src )[c];
			}
		}
		return out;
	};
}

// Four seconds: two words of half a second (at 0.5 and at 2.0) with a pause before, between and after
AudioBuffer four_seconds()
{
	AudioBuffer a( RATE * 4, RATE, 1 );
	for ( size_t f = 0; f < a.frames(); ++f )
	{
		const double t = static_cast<double>( f ) / RATE;
		const bool speech = ( t >= 0.5 && t < 1.0 ) || ( t >= 2.0 && t < 2.5 );
		a.frame( f )[0] = speech ? 0.5F : 0.F;
	}
	return a;
}

const std::vector<SpeechSpan> TWO_WORDS{ { 0.5, 1.0 }, { 2.0, 2.5 } };

size_t loud_frames( const AudioBuffer &a )
{
	size_t n = 0;
	for ( size_t f = 0; f < a.frames(); ++f )
	{
		if ( std::abs( a.frame( f )[0] - 0.5F ) < 1e-6F )
		{
			++n;
		}
	}
	return n;
}
}

TEST_CASE( "compress_segment shortens the pauses before it touches the speech", "[compress]" )
{
	// 1 second has to go; the pauses (0.5, 1.0 and 1.5 long, 0.08 to be kept of each) can give 2.76
	const Compressed c = compress_segment( four_seconds(), TWO_WORDS, 3.0, Stretcher{} ); // no stretcher: it must not be needed

	CHECK( c.audio.frames() == RATE * 3 );
	CHECK( c.tempo == 1.0 );

	SECTION( "the speech is all there, untouched" )
	{
		CHECK( loud_frames( c.audio ) == RATE ); // two words of half a second
	}

	SECTION( "each pause gave up its share, in proportion to what it could spare" )
	{
		// the pause before the first word: 0.5 - 1.0 * 0.42 / 2.76
		CHECK( c.map( 0.5 ) == Approx( 0.5 - 0.42 / 2.76 ).margin( 2.0 / RATE ) );
		CHECK( c.map( 1.0 ) == Approx( 1.0 - 0.42 / 2.76 ).margin( 2.0 / RATE ) );
		// the pause between the words: 1.0 - 1.0 * 0.92 / 2.76
		CHECK( c.map( 2.0 ) == Approx( 1.0 - 0.42 / 2.76 + 1.0 - 0.92 / 2.76 ).margin( 3.0 / RATE ) );
		CHECK( c.map( 4.0 ) == Approx( 3.0 ).margin( 3.0 / RATE ) );
	}
}

TEST_CASE( "compress_segment speeds the speech up by only what the pauses could not give", "[compress]" )
{
	int calls = 0;
	double tempo = 0.0;
	// 2.8 has to go but the pauses can only give 2.76: every pause goes to its floor (1.0 of speech + 3 * 0.08)
	const Compressed c = compress_segment( four_seconds(), TWO_WORDS, 1.2, fake_stretcher( &calls, &tempo ) );

	CHECK( c.audio.frames() == RATE * 6 / 5 );
	CHECK( calls == 1 );
	CHECK( tempo == Approx( 1.24 / 1.2 ).epsilon( 1e-3 ) ); // a 3% speed-up, not the 233% the whole thing would need
	CHECK( c.tempo == Approx( tempo ) );
	CHECK( c.map( 4.0 ) == Approx( 1.2 ).margin( 3.0 / RATE ) );
}

TEST_CASE( "compress_segment does not touch a segment that is short enough", "[compress]" )
{
	SECTION( "it is padded with silence" )
	{
		const Compressed c = compress_segment( four_seconds(), TWO_WORDS, 5.0, Stretcher{} );
		CHECK( c.audio.frames() == RATE * 5 );
		CHECK( c.map.identity() );
		CHECK( c.tempo == 1.0 );
		CHECK( loud_frames( c.audio ) == RATE );
		CHECK( c.audio.frame( RATE * 9 / 2 )[0] == 0.F );
	}

	SECTION( "exactly the right length is left as it is" )
	{
		const Compressed c = compress_segment( four_seconds(), TWO_WORDS, 4.0, Stretcher{} );
		CHECK( c.map.identity() );
		CHECK( loud_frames( c.audio ) == RATE );
	}
}

TEST_CASE( "compress_segment with no speech is all pause", "[compress]" )
{
	const AudioBuffer silence( RATE * 4, RATE, 1 );
	const Compressed c = compress_segment( silence, {}, 2.0, Stretcher{} );
	CHECK( c.audio.frames() == RATE * 2 );
	CHECK( c.tempo == 1.0 );
}

TEST_CASE( "compress_segment cannot speed up without something to do it", "[compress]" )
{
	// nothing but speech, so nothing can be got from the pauses
	AudioBuffer speech( RATE * 2, RATE, 1 );
	for ( size_t f = 0; f < speech.frames(); ++f ) speech.frame( f )[0] = 0.5F;
	CHECK_THROWS_AS( compress_segment( speech, { { 0.0, 2.0 } }, 1.0, Stretcher{} ), std::runtime_error );

	int calls = 0;
	const Compressed c = compress_segment( speech, { { 0.0, 2.0 } }, 1.0, fake_stretcher( &calls ) );
	CHECK( calls == 1 );
	CHECK( c.tempo == Approx( 2.0 ) );
	CHECK( c.audio.frames() == RATE );
}

TEST_CASE( "compress_segment copes with awkward speech spans", "[compress]" )
{
	SECTION( "overlapping, out of order, or past the ends" )
	{
		const Compressed c = compress_segment( four_seconds(), { { 2.0, 2.5 }, { 0.5, 1.2 }, { 0.9, 1.0 }, { 3.5, 9.0 } }, 3.0, fake_stretcher() );
		CHECK( c.audio.frames() == RATE * 3 );
	}

	SECTION( "empty and no spans" )
	{
		CHECK( compress_segment( four_seconds(), { { 1.0, 1.0 } }, 3.0, fake_stretcher() ).audio.frames() == RATE * 3 );
	}

	SECTION( "nothing to compress" )
	{
		CHECK( compress_segment( AudioBuffer(), {}, 1.0, Stretcher{} ).audio.frames() == RATE * 0 + 0 );
	}
}

TEST_CASE( "retime_audio shortens a segment that is given less time than it needs", "[compress]" )
{
	// three segments (0-1, 1-3, 3-4 seconds) of 0.1, 0.2 and 0.3
	AudioBuffer natural( RATE * 4, RATE, 1 );
	for ( size_t f = 0; f < natural.frames(); ++f )
	{
		natural.frame( f )[0] = f < RATE ? 0.1F : ( f < 3 * RATE ? 0.2F : 0.3F );
	}
	Timeline timeline;
	timeline.cuts = { { "\nstart", 0.0 }, { "a", 1.0 }, { "b", 3.0 } };
	timeline.end = 4.0;

	// the middle segment is given 1.0 second instead of 2.0
	const std::vector<double> starts{ 0.0, 1.0, 2.0, 3.0 };
	const std::vector<std::vector<SpeechSpan>> speech{ { { 0.0, 1.0 } }, { { 0.0, 2.0 } }, { { 0.0, 1.0 } } };
	const Retimed r = retime_audio( natural, timeline, starts, speech, fake_stretcher() );

	CHECK( r.audio.frames() == RATE * 3 );
	CHECK( r.audio.frame( RATE / 2 )[0] == Approx( 0.1F ) );
	CHECK( r.audio.frame( RATE * 3 / 2 )[0] == Approx( 0.2F ) );
	CHECK( r.audio.frame( RATE * 5 / 2 )[0] == Approx( 0.3F ) );

	REQUIRE( r.tempos.size() == 3 );
	CHECK( r.tempos[0] == 1.0 );
	CHECK( r.tempos[1] == Approx( 2.0 ) );
	CHECK( r.tempos[2] == 1.0 );

	CHECK( r.maps[0].identity() );
	CHECK_FALSE( r.maps[1].identity() );
	CHECK( r.maps[1]( 2.0 ) == Approx( 1.0 ) );
	CHECK( r.maps[1]( 1.0 ) == Approx( 0.5 ) );
	CHECK( r.maps[2].identity() );
}

TEST_CASE( "stretch_with_ffmpeg changes the length and not the pitch", "[compress][ffmpeg]" )
{
	if ( std::system( "ffmpeg -version > /dev/null 2>&1" ) != 0 )
	{
		SKIP( "ffmpeg is not installed" );
	}

	constexpr size_t SR = 48000;
	AudioBuffer tone( SR * 2, SR, 1 ); // two seconds of 440 Hz
	for ( size_t f = 0; f < tone.frames(); ++f )
	{
		tone.frame( f )[0] = 0.5F * std::sin( 2.F * std::numbers::pi_v<float> * 440.F * static_cast<float>( f ) / static_cast<float>( SR ) );
	}

	const AudioBuffer fast = stretch_with_ffmpeg( tone, 1.25 );

	CHECK( fast.rate() == SR );
	CHECK( fast.channels() == 1 );
	// a fifth shorter: 1.6 seconds (the filter may be a few tens of milliseconds out)
	CHECK( static_cast<double>( fast.frames() ) / SR == Approx( 1.6 ).margin( 0.05 ) );

	// the pitch is the same: about 440 rising crossings a second, not 550
	size_t crossings = 0;
	for ( size_t f = SR / 4 + 1; f < fast.frames() - SR / 4; ++f )
	{
		if ( fast.frame( f - 1 )[0] < 0.F && fast.frame( f )[0] >= 0.F )
		{
			++crossings;
		}
	}
	const double seconds = static_cast<double>( fast.frames() - SR / 2 ) / SR;
	CHECK( static_cast<double>( crossings ) / seconds == Approx( 440.0 ).epsilon( 0.02 ) );

	SECTION( "a tempo of 1 leaves it alone, and it works on more than two channels' worth of change" )
	{
		CHECK( stretch_with_ffmpeg( tone, 1.0 ).frames() == tone.frames() );
		const AudioBuffer very_fast = stretch_with_ffmpeg( tone, 3.0 ); // needs two atempo in a row
		CHECK( static_cast<double>( very_fast.frames() ) / SR == Approx( 2.0 / 3.0 ).margin( 0.1 ) );
	}
}
