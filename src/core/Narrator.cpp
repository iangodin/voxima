
#include "Narrator.h"

#include <algorithm>
#include <audio/Resample.h>
#include <core/Stage.h>
#include <cstring>

void Narrator::setup( std::string_view name, std::string_view lang, Script &script, DubbingStage &stage )
{
	_language = lang;
	_mic = stage.mic( lang );
	_voice_ref = stage.voice_dir() / script.fetch( lang, "voices", name ).as_string();
	_script = script;
	_tts = stage.tts();
	_audio_rate = stage.rate();
	_buffer = AudioBuffer( stage.audio_block(), _audio_rate, 1 );
}

double Narrator::speech( const std::vector<std::string> &text )
{
	double duration = 0.0;

	_results.clear();

	for ( const auto &t: text )
	{
		auto result = _tts->synthesize( t, _voice_ref, _language );
		if ( !result.success || !result.audio.has_value() )
		{
			throw std::runtime_error( "narration speech synthesis failed: " + result.message );
		}

		// Resample immediately if the rate doesn't match the mixer's rate
		if ( result.audio->rate() != _audio_rate )
		{
			result.audio = resample( result.audio.value(), _audio_rate );
		}
		duration += result.audio->duration();

		_results.push_back( std::move( result ) );
	}
	_current_result_index = 0;
	_offset = 0;

	return duration;
}

void Narrator::speak( size_t )
{
	size_t recorded = 0;

	while ( recorded < _buffer.frames() && _current_result_index < _results.size() )
	{
		const auto &result = _results[_current_result_index];
		const auto &audio = result.audio.value();

		if ( _offset >= audio.frames() )
		{
			_current_result_index++;
			_offset = 0;
			continue;
		}

		const size_t remaining_samples = _buffer.frames() - recorded;
		const size_t frames_to_record = std::min( remaining_samples, audio.frames() - _offset );
		if ( frames_to_record > 0 )
		{
			const size_t samples = frames_to_record * audio.channels();
			std::memcpy( _buffer.frame( recorded ), audio.frame( _offset ), samples * sizeof( float ) );
			_offset += frames_to_record;
			recorded += frames_to_record;
		}

		if ( _offset >= audio.frames() )
		{
			_current_result_index++;
			_offset = 0;
		}
	}

	if ( recorded < _buffer.frames() )
	{
		std::fill( _buffer.frame( recorded ), _buffer.end(), 0.F );
	}
	_mic->record( _buffer );
}

bool Narrator::is_done() const
{
	if ( _current_result_index >= _results.size() )
	{
		return true;
	}

	// If we're at the last result, check if it's done
	if ( _current_result_index == _results.size() - 1 )
	{
		const auto &result = _results[_current_result_index];
		if ( !result.success || !result.audio.has_value() )
		{
			return true;
		}
		return _offset >= result.audio.value().frames();
	}

	return false;
}

