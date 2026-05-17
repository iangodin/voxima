
#pragma once

#include "AudioBuffer.h"

#include <sndfile.h>
#include <string>
#include <string_view>

class AudioMixer
{
public:
	AudioMixer();
	~AudioMixer();

	bool open( const std::string &filename, size_t rate, size_t block_size );

	void close();

	void add_audio( const AudioBuffer &buffer );
	void write_audio( void );

	size_t rate( void ) const
	{
		return _buffer.rate();
	}

	static bool supported_format( const std::string &filename );
	static bool supported_codec( std::string_view codec );

private:
	SNDFILE *_sndfile = nullptr;
	AudioBuffer _buffer;
};
