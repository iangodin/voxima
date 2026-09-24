#pragma once

#include "Script.h"

#include <tts/TTSClient.h>

#include <audio/DubbingStage.h>
#include <audio/Microphone.h>

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
	AudioBuffer _buffer;
	ObserverPtr<Script> _script;
	ObserverPtr<tts::TTSClient> _tts;
	ObserverPtr<Microphone> _mic;
	ObserverPtr<DubbingStage> _stage;
	std::vector<tts::SynthesisResult> _results;
	std::vector<PendingCue> _pending_cues;
	bool _cues_emitted = true;
	size_t _audio_rate = 0;
	size_t _current_result_index = 0;
	size_t _offset = 0;
};
