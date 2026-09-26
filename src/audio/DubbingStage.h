
#pragma once

#include "AudioMixer.h"
#include "Microphone.h"
#include "SubtitleCue.h"

#include <tts/TTSClient.h>

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

class DubbingStage
{
public:
	// Setup basic rendering parameters. tts_model is the Chatterbox GGUF to
	// load, tts_voice_dir is the base directory reference-clip paths in
	// scripts' voices:: blocks resolve against. aligner_model is optional
	// (empty = no subtitle timing available) - the Qwen3 Forced Aligner GGUF
	// used to produce word-level timestamps for subtitle cues.
	void setup(
		const std::filesystem::path &render_dir,
		std::string_view codec,
		size_t audio_rate,
		size_t video_rate,
		const std::filesystem::path &tts_model,
		std::filesystem::path tts_voice_dir,
		const std::string &tts_backend,
		int tts_threads,
		const std::filesystem::path &aligner_model = {} );

	// Start a fresh set of tracks, one per language, writing into render_dir (created if need be): what a render
	// does, after setup() has loaded the models once. Everything from a previous render (mixers, subtitles) is dropped.
	// With in_memory the audio is kept in memory, not written to files (see take_audio()).
	void begin( const std::filesystem::path &render_dir, const std::vector<std::string> &languages, bool in_memory = false );

	// Add a language track
	void add_language( std::string_view lang, bool in_memory = false );

	// The interleaved stereo float samples mixed since the last call, for a track begun in memory
	std::vector<float> take_audio( std::string_view lang );

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

	// Appends one subtitle cue for a language track. Called by Narrator once
	// a narrated line's absolute timing (anchored to the real output audio
	// position) is known.
	void caption( std::string_view lang, SubtitleCue cue );

	// Accumulated subtitle cues for a language, in the order they were
	// captioned. Empty (not an error) for a language with no narration or
	// where alignment/synthesis isn't supported (e.g. Polish alignment, or
	// zh/ja synthesis - see tts.md's accepted-gap notes).
	const std::vector<SubtitleCue> &subtitles( std::string_view lang ) const;

	// How much the other languages' narration may be speeded up to keep them in step with the main one (see
	// Narration::synchronise): 1.0, the default, not at all, the slowest language sets the pace and the others
	// are padded with silence. Speeding up needs ffmpeg.
	void set_max_speedup( double speedup )
	{
		_max_speedup = speedup;
	}

	double max_speedup() const
	{
		return _max_speedup;
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
	std::map<std::string, std::vector<SubtitleCue>, std::less<>> _subtitles;
	std::string _codec;
	double _max_speedup = 1.0;
};
