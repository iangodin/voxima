#include "Narration.h"
#include <core/Stage.h>
#include <audio/Resample.h>
#include <cstring>
#include <algorithm>

namespace tts
{

void Narration::synthesize(
	Stage &stage,
	const std::string &speaker,
	const std::string &language,
	const std::vector<std::string> &text )
{
	TTSClient client( stage.io_context, "127.0.0.1", 5555, stage.tts_cache );
	_results.clear();
	for ( const auto &t: text )
	{
		auto result = client.synthesize( t, speaker, language );
		if ( !result.success )
		{
			throw std::runtime_error( "Narration synthesis failed: " + result.message );
		}

		// Resample immediately if the rate doesn't match the mixer's rate
		if ( result.audio.has_value() && result.audio->rate() != stage.audio.rate() )
		{
			result.audio = resample( result.audio.value(), stage.audio.rate() );
		}

		_results.push_back( std::move( result ) );
	}
	_current_result_index = 0;
	_offset = 0;
}

void Narration::speak( Microphone &mic, size_t samples )
{
	size_t remaining_samples = samples;

	while ( remaining_samples > 0 && _current_result_index < _results.size() )
	{
		const auto &result = _results[_current_result_index];
		if ( !result.success || !result.audio.has_value() )
		{
			_current_result_index++;
			_offset = 0;
			continue;
		}

		const auto &audio = result.audio.value();
		if ( _offset >= audio.frames() )
		{
			_current_result_index++;
			_offset = 0;
			continue;
		}

		size_t frames_to_record = std::min( remaining_samples, audio.frames() - _offset );
		if ( frames_to_record > 0 )
		{
			AudioBuffer chunk( frames_to_record, audio.rate(), audio.channels() );
			std::memcpy(
				chunk.data(),
				audio.frame( _offset ),
				frames_to_record * audio.channels() * sizeof( float ) );
			mic.record( chunk );
			_offset += frames_to_record;
			remaining_samples -= frames_to_record;
		}

		if ( _offset >= audio.frames() )
		{
			_current_result_index++;
			_offset = 0;
		}
	}

	// Fill remaining samples with silence if we ran out of audio but still have sample quota
	if ( remaining_samples > 0 )
	{
		AudioBuffer silence( remaining_samples, 48000, 2 );
		std::fill( silence.data(), silence.data() + silence.samples(), 0.0f );
		mic.record( silence );
	}
}

bool Narration::is_done() const
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

}
