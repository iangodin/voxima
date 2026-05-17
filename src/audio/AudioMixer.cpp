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

bool AudioMixer::open( const std::string &filename, size_t rate, size_t default_frame_size )
{
	close();
	_rate = rate;
	_default_frame_size = default_frame_size;

	SF_INFO sfinfo = {};
	sfinfo.channels = 2;
	sfinfo.samplerate = static_cast<int>( _rate );

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
	if ( buffer.channels() != 2 )
	{
		return;
	}

	if ( _mixed_buffer.size() < buffer.samples() )
	{
		_mixed_buffer.resize( buffer.samples(), 0.0f );
	}

	_current_frame_size = std::max( _current_frame_size, buffer.frames() );

	const float *data = buffer.data();
	for ( size_t i = 0; i < buffer.samples(); ++i )
	{
		_mixed_buffer[i] += data[i];
	}
}

void AudioMixer::save_audio()
{
	if ( !_sndfile )
	{
		return;
	}

	size_t frames_to_write = _current_frame_size;
	if ( frames_to_write == 0 )
	{
		frames_to_write = _default_frame_size;
	}

	if ( _mixed_buffer.size() < frames_to_write * 2 )
	{
		_mixed_buffer.resize( frames_to_write * 2, 0.0f );
	}

	sf_writef_float( _sndfile, _mixed_buffer.data(), static_cast<sf_count_t>( frames_to_write ) );

	std::fill( _mixed_buffer.begin(), _mixed_buffer.end(), 0.0f );
	_current_frame_size = 0;
}

bool AudioMixer::supported_format( const std::string &filename )
{
	return filename.ends_with( ".wav" ) || filename.ends_with( ".flac" );
}
