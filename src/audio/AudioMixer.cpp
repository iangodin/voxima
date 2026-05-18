#include "AudioMixer.h"

#include <algorithm>
#include <iostream>

AudioMixer::AudioMixer()
{
}

AudioMixer::~AudioMixer()
{
	close();
}

bool AudioMixer::open( const std::string &filename, size_t rate, size_t block_size )
{
	close();

	SF_INFO sfinfo = {};
	sfinfo.channels = 2;
	sfinfo.samplerate = static_cast<int>( rate );

	_buffer = AudioBuffer( block_size, rate, 2 );

	if ( filename.ends_with( ".wav" ) )
	{
		sfinfo.format = SF_FORMAT_WAVEX | SF_FORMAT_PCM_16;
	}
	else if ( filename.ends_with( ".flac" ) )
	{
		sfinfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
	}
	else
	{
		throw std::runtime_error( "unknown audio file format " + filename );
	}

	_sndfile = sf_open( filename.c_str(), SFM_WRITE, &sfinfo );
	if ( !_sndfile )
	{
		throw std::runtime_error(
			"failed to open audio file " + filename + ": " + sf_strerror( nullptr ) );
	}

	if ( ( sfinfo.format & SF_FORMAT_TYPEMASK ) != SF_FORMAT_FLAC )
	{
		int channel_map[2] = { SF_CHANNEL_MAP_LEFT, SF_CHANNEL_MAP_RIGHT };
		if ( sf_command( _sndfile, SFC_SET_CHANNEL_MAP_INFO, channel_map, sizeof( channel_map ) ) ==
			 SF_FALSE )
		{
			throw std::runtime_error( "audio format does not support explicit channel maps." );
		}
	}
	else
	{
		double compression_level = 1.0;
		if ( sf_command( _sndfile, SFC_SET_COMPRESSION_LEVEL, &compression_level, sizeof( compression_level ) ) == SF_FALSE )
		{
			throw std::runtime_error( "unable to set FLAC compression level" );
		}
	}

	return true;
}

void AudioMixer::close()
{
	if ( _sndfile )
	{
		sf_command( _sndfile, SFC_UPDATE_HEADER_NOW, nullptr, 0 );

		sf_close( _sndfile );
		_sndfile = nullptr;
	}
}

void AudioMixer::add_audio( const AudioBuffer &buffer )
{
	if ( !_buffer.same( buffer ) )
	{
		throw std::runtime_error( "expected audio buffer to match mixer output" );
	}

	std::transform( buffer.begin(), buffer.end(), _buffer.begin(), _buffer.begin(), std::plus{} );
}

void AudioMixer::write_audio( void )
{
	if ( !_sndfile )
	{
		return;
	}

	sf_writef_float( _sndfile, _buffer.data(), static_cast<sf_count_t>( _buffer.frames() ) );

	std::fill( _buffer.begin(), _buffer.end(), 0.F );
}

bool AudioMixer::supported_codec( std::string_view codec )
{
	return codec == "wav" || codec == "flac";
}
