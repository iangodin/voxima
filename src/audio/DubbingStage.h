
#pragma once

#include "AudioMixer.h"
#include "Microphone.h"

#include <tts/asio_wrapper.h>
#include <tts/TTSClient.h>

#include <filesystem>
#include <map>
#include <string>
#include <optional>
#include <filesystem>

class DubbingStage
{
public:
	// Setup basic rendering parameters
	void setup( const std::filesystem::path &render_dir, size_t audio_rate, size_t video_rate );

	// Add a language track
	void add_language( std::string_view lang );

	void add_audio( const AudioBuffer &buffer );

	Microphone &mic( std::string_view lang );

	// Save current audio buffer (corresponding to one video frame)
	void write_audio();

	size_t rate() const
	{
		return _audio_rate;
	}

	size_t audio_block() const
	{
		return _block_size;
	}

	tts::TTSClient &tts()
	{
		return _tts;
	}

	void close();

private:
	size_t _audio_rate;
	size_t _block_size;
	std::filesystem::path _render_dir;
	asio::io_context io_context;
	tts::TTSClient _tts{ io_context, "127.0.0.1", 5555 };
	std::map<std::string, AudioMixer, std::less<>> _mixers;
	std::map<std::string, Microphone, std::less<>> _mics;
};

