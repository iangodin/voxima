
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
	_stage = stage;
	_audio_rate = stage.rate();
	_buffer = AudioBuffer( stage.audio_block(), _audio_rate, 1 );
}

double Narrator::speech( const std::vector<std::string> &text )
{
	double duration = 0.0;

	_results.clear();
	_pending_cues.clear();
	_cues_emitted = false;

	double cumulative = 0.0;
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
		const double line_duration = result.audio->duration();
		duration += line_duration;

		// Default cue span is the full synthesized line; refined below by
		// forced alignment when available, trimming leading/trailing
		// silence the model left in. Falls back gracefully for languages
		// without alignment support (e.g. Polish - see tts.md).
		double local_start = cumulative;
		double local_end = cumulative + line_duration;
		if ( _tts->has_aligner() )
		{
			try
			{
				auto words = _tts->align( t, result.audio.value(), _language );
				if ( !words.empty() )
				{
					local_start = cumulative + words.front().start_seconds;
					local_end = cumulative + words.back().end_seconds;
				}
			}
			catch ( const std::exception & )
			{
				// Alignment unsupported/failed for this line's language -
				// keep the full-buffer fallback span computed above.
			}
		}
		_pending_cues.push_back( PendingCue{ local_start, local_end, t } );
		cumulative += line_duration;

		_results.push_back( std::move( result ) );
	}
	_current_result_index = 0;
	_offset = 0;

	return duration;
}

void Narrator::speak( size_t )
{
	// On the first speak() call of a batch, the mixer's current
	// frames_written() is the absolute sample position this frame's audio
	// will land at (write_audio() - which advances that counter - hasn't
	// run yet this tick). That's the ground-truth anchor for converting
	// this batch's local cue offsets into absolute output-stream time;
	// naive cumulative-duration timing would be wrong whenever narration
	// doesn't start exactly at the top of the render.
	if ( !_cues_emitted && !_results.empty() )
	{
		const double anchor_seconds =
			static_cast<double>( _mic->frames_written() ) / static_cast<double>( _audio_rate );
		for ( const auto &cue: _pending_cues )
		{
			_stage->caption(
				_language,
				SubtitleCue{ anchor_seconds + cue.local_start, anchor_seconds + cue.local_end, cue.text } );
		}
		_cues_emitted = true;
	}

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

