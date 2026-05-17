#pragma once

#include "Script.h"

#include <tts/TTSClient.h>

#include <audio/DubbingStage.h>
#include <audio/Microphone.h>

#include <utils/ObserverPtr.h>

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
	std::string _voice;
	std::string _language;
	AudioBuffer _buffer;
	ObserverPtr<Script> _script;
	ObserverPtr<tts::TTSClient> _tts;
	ObserverPtr<Microphone> _mic;
	std::vector<tts::SynthesisResult> _results;
	size_t _audio_rate = 0;
	size_t _current_result_index = 0;
	size_t _offset = 0;
};
