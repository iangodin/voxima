#pragma once

#include "Script.h"

#include <tts/TTSClient.h>

#include <audio/DubbingStage.h>
#include <audio/Microphone.h>
#include <audio/TimedWord.h>

#include <utils/ObserverPtr.h>

#include <filesystem>
#include <string>
#include <vector>

struct Stage;

class Narrator
{
public:
	void setup( std::string_view name, std::string_view lang, Script &script, DubbingStage &stage );

	double speech( const std::vector<std::string> &text );

	void speak( size_t samples );

	const std::string language( void ) const
	{
		return _language;
	}

	bool is_done() const;

	// Words of the current speech() batch, timed relative to the batch start.
	// Real forced-alignment times when available, otherwise distributed
	// proportionally across each line (see distribute_words()).
	const std::vector<TimedWord> &words() const
	{
		return _words;
	}

	// Seconds of this batch's audio consumed by speak() so far. Compare
	// against TimedWord times to know which word is currently being spoken.
	double position() const
	{
		return static_cast<double>( _position_frames ) / static_cast<double>( _audio_rate );
	}

private:
	// Local (batch-relative) subtitle cue span for one narrated line, held
	// until the batch's absolute-time anchor is known (see speak()).
	struct PendingCue
	{
		double local_start;
		double local_end;
		std::string text;
	};

	std::filesystem::path _voice_ref;
	std::string _language;
	std::string _tts_language;
	AudioBuffer _buffer;
	ObserverPtr<Script> _script;
	ObserverPtr<tts::TTSClient> _tts;
	ObserverPtr<Microphone> _mic;
	ObserverPtr<DubbingStage> _stage;
	std::vector<tts::SynthesisResult> _results;
	std::vector<PendingCue> _pending_cues;
	std::vector<TimedWord> _words;
	size_t _position_frames = 0;
	bool _cues_emitted = true;
	size_t _audio_rate = 0;
	size_t _current_result_index = 0;
	size_t _offset = 0;
};
