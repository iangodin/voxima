
#pragma once

#include <string>
#include <vector>
#include <sndfile.h>
#include "AudioBuffer.h"

class AudioMixer
{
public:
	AudioMixer();
	~AudioMixer();

	bool open( const std::string &filename, size_t rate = 48000, size_t default_frame_size = 2000 );
	void close();

	void add_audio( const AudioBuffer &buffer );
	void save_audio();

private:
	SNDFILE *_sndfile = nullptr;
	std::vector<float> _mixed_buffer;
	size_t _rate = 48000;
	size_t _default_frame_size = 2000;
	size_t _current_frame_size = 0;
};

