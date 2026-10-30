#include "Stretch.h"

#include <sndfile.h>

#include <atomic>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

namespace
{

// A path for a temporary file that no other call (or process) is using
std::filesystem::path temporary_path( const std::string &suffix )
{
	static std::atomic<unsigned> counter{ 0 };
	std::ostringstream name;
	name << "sonimation-stretch-" << GETPID() << "-" << counter++ << suffix;
	return std::filesystem::temp_directory_path() / name.str();
}

// Removes the file when it goes out of scope, whatever happens
struct Cleanup
{
	std::filesystem::path path;

	~Cleanup()
	{
		std::error_code ignored;
		std::filesystem::remove( path, ignored );
	}
};

void write_float_wav( const std::filesystem::path &path, const AudioBuffer &audio )
{
	SF_INFO info = {};
	info.channels = static_cast<int>( audio.channels() );
	info.samplerate = static_cast<int>( audio.rate() );
	info.format = SF_FORMAT_WAV | SF_FORMAT_FLOAT;

	SNDFILE *file = sf_open( path.string().c_str(), SFM_WRITE, &info );
	if ( !file )
	{
		throw std::runtime_error( "stretch: could not write " + path.string() );
	}
	sf_writef_float( file, audio.data(), static_cast<sf_count_t>( audio.frames() ) );
	sf_close( file );
}

AudioBuffer read_wav( const std::filesystem::path &path )
{
	SF_INFO info = {};
	SNDFILE *file = sf_open( path.string().c_str(), SFM_READ, &info );
	if ( !file )
	{
		throw std::runtime_error( "stretch: ffmpeg made no output (" + path.string() + ")" );
	}
	AudioBuffer out( static_cast<size_t>( info.frames ), static_cast<size_t>( info.samplerate ), static_cast<size_t>( info.channels ) );
	sf_readf_float( file, out.data(), info.frames );
	sf_close( file );
	return out;
}

// atempo takes 0.5 to 2 in older ffmpegs; a larger change is several of them in a row
std::string tempo_filter( double tempo )
{
	std::ostringstream filter;
	filter.precision( 6 );
	filter << std::fixed;
	bool first = true;
	while ( tempo > 2.0 )
	{
		filter << ( first ? "" : "," ) << "atempo=2.0";
		tempo /= 2.0;
		first = false;
	}
	while ( tempo < 0.5 )
	{
		filter << ( first ? "" : "," ) << "atempo=0.5";
		tempo /= 0.5;
		first = false;
	}
	filter << ( first ? "" : "," ) << "atempo=" << tempo;
	return filter.str();
}

}

AudioBuffer stretch_with_ffmpeg( const AudioBuffer &audio, double tempo )
{
	if ( audio.empty() || tempo <= 0.0 || std::abs( tempo - 1.0 ) < 1e-6 )
	{
		return audio;
	}

	const Cleanup input{ temporary_path( ".wav" ) };
	const Cleanup output{ temporary_path( ".wav" ) };
	write_float_wav( input.path, audio );

	std::ostringstream command;
	command << "ffmpeg -y -hide_banner -loglevel error -i \"" << input.path.string() << "\" -filter:a \"" << tempo_filter( tempo )
			<< "\" -c:a pcm_f32le \"" << output.path.string() << "\"";
	if ( std::system( command.str().c_str() ) != 0 )
	{
		throw std::runtime_error( "stretch: ffmpeg could not be run (speeding up a language needs it installed): " + command.str() );
	}

	return read_wav( output.path );
}
