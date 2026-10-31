#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <audio/Sound.h>
#include <audio/SoundFile.h>
#include <audio/Volume.h>

#include <sndfile.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>

using Catch::Matchers::WithinAbs;
using namespace std::chrono_literals;

namespace
{
constexpr size_t RATE = 48000;

// A stereo sound of `frames` sample frames, all `value` (left) and -`value` (right)
std::shared_ptr<const AudioBuffer> constant( size_t frames, float value = 0.5F )
{
	auto audio = std::make_shared<AudioBuffer>( frames, RATE, 2 );
	for ( size_t f = 0; f < frames; ++f )
	{
		audio->frame( f )[0] = value;
		audio->frame( f )[1] = -value;
	}
	return audio;
}

AudioBuffer block( size_t frames )
{
	return AudioBuffer( frames, RATE, 2 );
}
} // namespace

TEST_CASE( "a sound file is read, made stereo and resampled", "[sound]" )
{
	// A second of mono at 44.1 kHz, written as FLAC
	const auto path = std::filesystem::temp_directory_path() / "voxima-test-sound.flac";
	{
		SF_INFO info = {};
		info.channels = 1;
		info.samplerate = 44100;
		info.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
		SNDFILE *file = sf_open( path.string().c_str(), SFM_WRITE, &info );
		REQUIRE( file );
		std::vector<float> samples( 44100 );
		for ( size_t i = 0; i < samples.size(); ++i )
			samples[i] = 0.5F * std::sin( 2.F * 3.14159265F * 440.F * static_cast<float>( i ) / 44100.F );
		sf_writef_float( file, samples.data(), static_cast<sf_count_t>( samples.size() ) );
		sf_close( file );
	}

	const AudioBuffer raw = read_sound( path );
	CHECK( raw.rate() == 44100 );
	CHECK( raw.channels() == 1 );
	CHECK( raw.frames() == 44100 );

	const AudioBuffer ready = load_sound( path, RATE );
	CHECK( ready.rate() == RATE );
	CHECK( ready.channels() == 2 );
	CHECK( ready.frames() >= 47990 );
	CHECK( ready.frames() <= 48010 );
	for ( size_t f = 0; f < ready.frames(); f += 997 )
	{
		CHECK( ready.frame( f )[0] == ready.frame( f )[1] );
	}

	std::filesystem::remove( path );
	CHECK_THROWS( read_sound( path ) );
}

TEST_CASE( "a sound plays at its volume, then ends", "[sound]" )
{
	PlayingSound sound( constant( 1000 ), { .volume = -6.0 } );

	AudioBuffer out = block( 600 );
	sound.mix_into( out );
	CHECK_THAT( out.frame( 0 )[0], WithinAbs( 0.5 * db_to_gain( -6.F ), 1e-6 ) );
	CHECK_THAT( out.frame( 599 )[1], WithinAbs( -0.5 * db_to_gain( -6.F ), 1e-6 ) );
	CHECK_FALSE( sound.done() );

	out = block( 600 );
	sound.mix_into( out );
	CHECK( out.frame( 399 )[0] != 0.F );
	CHECK( out.frame( 400 )[0] == 0.F ); // it has ended
	CHECK( sound.done() );
}

TEST_CASE( "sounds add together", "[sound]" )
{
	PlayingSound a( constant( 100, 0.25F ), {} );
	PlayingSound b( constant( 100, 0.5F ), {} );
	AudioBuffer out = block( 10 );
	a.mix_into( out );
	b.mix_into( out );
	CHECK_THAT( out.frame( 5 )[0], WithinAbs( 0.75, 1e-6 ) );
}

TEST_CASE( "a looping sound starts again at its end", "[sound]" )
{
	auto audio = std::make_shared<AudioBuffer>( 4, RATE, 2 );
	for ( size_t f = 0; f < 4; ++f )
		audio->frame( f )[0] = static_cast<float>( f + 1 );

	PlayingSound sound( audio, { .loop = true } );
	AudioBuffer out = block( 10 );
	sound.mix_into( out );
	const float expected[] = { 1, 2, 3, 4, 1, 2, 3, 4, 1, 2 };
	for ( size_t f = 0; f < 10; ++f )
		CHECK( out.frame( f )[0] == expected[f] );
	CHECK_FALSE( sound.done() );

	sound.stop();
	CHECK( sound.done() );
}

TEST_CASE( "a sound can start part way in", "[sound]" )
{
	auto audio = std::make_shared<AudioBuffer>( RATE, RATE, 2 );
	for ( size_t f = 0; f < RATE; ++f )
		audio->frame( f )[0] = static_cast<float>( f );

	PlayingSound sound( audio, { .from = 0.5s } );
	CHECK_THAT( sound.position(), WithinAbs( 0.5, 1e-9 ) );
	AudioBuffer out = block( 1 );
	sound.mix_into( out );
	CHECK( out.frame( 0 )[0] == static_cast<float>( RATE / 2 ) );
}

TEST_CASE( "a fade in rises smoothly from silence to the volume", "[sound]" )
{
	PlayingSound sound( constant( RATE * 2, 1.F ), { .volume = -10.0, .fade_in = 1s } );
	AudioBuffer out = block( RATE + 10 );
	sound.mix_into( out );

	CHECK( out.frame( 0 )[0] < 0.002F ); // -60 dB, near enough nothing
	float last = 0.F;
	for ( size_t f = 0; f < RATE; f += 480 )
	{
		CHECK( out.frame( f )[0] >= last );
		last = out.frame( f )[0];
	}
	CHECK_THAT( out.frame( RATE + 5 )[0], WithinAbs( db_to_gain( -10.F ), 1e-5 ) );
	CHECK_THAT( sound.volume(), WithinAbs( -10.0, 1e-9 ) );
}

TEST_CASE( "a volume change ramps, and a fade out ends the sound", "[sound]" )
{
	PlayingSound sound( constant( RATE * 10, 1.F ), {} );

	sound.set_volume( -20.0, 0.1s );
	AudioBuffer out = block( RATE / 5 );
	sound.mix_into( out );
	CHECK( out.frame( 100 )[0] < 1.F );
	CHECK( out.frame( 100 )[0] > db_to_gain( -20.F ) );
	CHECK_THAT( out.frame( RATE / 5 - 1 )[0], WithinAbs( db_to_gain( -20.F ), 1e-5 ) );

	sound.fade_out( 0.5s );
	out = block( RATE );
	sound.mix_into( out );
	CHECK( sound.done() );
	CHECK( out.frame( RATE / 4 )[0] < db_to_gain( -20.F ) );
	CHECK( out.frame( RATE / 2 + 10 )[0] == 0.F ); // over

	PlayingSound now( constant( 100 ), {} );
	now.fade_out( 0s );
	CHECK( now.done() );
}
