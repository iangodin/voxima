
#pragma once

#include "AudioBuffer.h"

#include <string>
#include <string_view>
#include <vector>

struct sf_private_tag; // libsndfile's file (SNDFILE is this)

class AudioMixer
{
public:
	AudioMixer();
	~AudioMixer();

	bool open( const std::string &filename, size_t rate, size_t block_size );

	// Keep the mixed audio in memory instead of writing a file (for a live preview): write_audio() appends
	// each block, and take_memory() hands over what has been written since the last call.
	void open_memory( size_t rate, size_t block_size );
	std::vector<float> take_memory();

	void close();

	void add_audio( const AudioBuffer &buffer );
	void write_audio( void );

	size_t rate( void ) const
	{
		return _buffer.rate();
	}

	// Total frames written to the output file so far - the ground truth for
	// "current absolute position in this language's output audio stream",
	// since write_audio() is called exactly once per video frame regardless
	// of whether that frame carried real narration or silence.
	size_t frames_written( void ) const
	{
		return _frames_written;
	}

	static bool supported_format( const std::string &filename );
	static bool supported_codec( std::string_view codec );

private:
	sf_private_tag *_sndfile = nullptr;
	AudioBuffer _buffer;
	bool _in_memory = false;
	std::vector<float> _memory;
	size_t _frames_written = 0;
};
