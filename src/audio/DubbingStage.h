
#pragma once

#include "AudioMixer.h"
#include "Microphone.h"

#include <tts/TTSClient.h>

#include <filesystem>
#include <map>
#include <optional>
#include <string>

class DubbingStage
{
public:
	// Setup basic rendering parameters. tts_model is the Chatterbox GGUF to
	// load, tts_voice_dir is the base directory reference-clip paths in
	// scripts' voices:: blocks resolve against.
	void setup(
		const std::filesystem::path &render_dir,
		std::string_view codec,
		size_t audio_rate,
		size_t video_rate,
		const std::filesystem::path &tts_model,
		std::filesystem::path tts_voice_dir,
		const std::string &tts_backend,
		int tts_threads );

	// Add a language track
	void add_language( std::string_view lang );

	void add_audio( const AudioBuffer &buffer );

	Microphone &mic( std::string_view lang );

	const std::string &output( std::string_view lang );

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
		return _tts.value();
	}

	const std::filesystem::path &voice_dir() const
	{
		return _tts_voice_dir;
	}

	void close();

private:
	size_t _audio_rate;
	size_t _block_size;
	std::filesystem::path _render_dir;
	std::filesystem::path _tts_voice_dir;
	std::optional<tts::TTSClient> _tts;
	std::map<std::string, AudioMixer, std::less<>> _mixers;
	std::map<std::string, Microphone, std::less<>> _mics;
	std::map<std::string, std::string, std::less<>> _outputs;
	std::string _codec;
};
