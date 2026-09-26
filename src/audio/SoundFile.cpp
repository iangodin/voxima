#include "SoundFile.h"

#include "Resample.h"

#include <sndfile.h>

#include <stdexcept>

AudioBuffer read_sound( const std::filesystem::path &path )
{
	SF_INFO info = {};
	SNDFILE *file = sf_open( path.string().c_str(), SFM_READ, &info );
	if ( !file )
	{
		throw std::runtime_error( "could not read sound " + path.string() + ": " + sf_strerror( nullptr ) );
	}

	AudioBuffer audio( static_cast<size_t>( info.frames ), static_cast<size_t>( info.samplerate ), static_cast<size_t>( info.channels ) );
	const sf_count_t read = sf_readf_float( file, audio.data(), info.frames );
	sf_close( file );

	// Compressed formats may know their length only roughly
	audio.resize( static_cast<size_t>( std::max<sf_count_t>( 0, read ) ) );
	return audio;
}

AudioBuffer to_stereo( const AudioBuffer &audio )
{
	if ( audio.channels() == 2 )
	{
		return audio;
	}

	AudioBuffer stereo( audio.frames(), audio.rate(), 2 );
	for ( size_t f = 0; f < audio.frames(); ++f )
	{
		const float *in = audio.frame( f );
		float *out = stereo.frame( f );
		out[0] = in[0];
		out[1] = audio.channels() == 1 ? in[0] : in[1];
	}
	return stereo;
}

AudioBuffer load_sound( const std::filesystem::path &path, size_t rate )
{
	return resample( to_stereo( read_sound( path ) ), rate );
}
