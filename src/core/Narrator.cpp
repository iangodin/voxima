
#include "Narrator.h"

#include <algorithm>
#include <audio/Resample.h>
#include <audio/Retime.h>
#include <audio/Stretch.h>
#include <core/Stage.h>
#include <cstring>
#include <utils/Language.h>
#include <utils/Markers.h>

void Narrator::setup( std::string_view name, std::string_view lang, Script &script, DubbingStage &stage )
{
	_language = lang;
	// Chatterbox's language allowlist is bare ISO 639-1 primary subtags (en,
	// fr, de, ...), not full BCP-47 tags - a regional variant like "en-GB"
	// or "fr-CA" would otherwise be rejected outright even though the base
	// language is fully supported. _language stays the full requested tag
	// for script/voice lookup (Script::fetch keys on it verbatim).
	_tts_language = language::primary_subtag( lang );
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
	_words.clear();
	_markers.clear();
	_timeline = {};
	_timeline.cuts.push_back( { "\nstart", 0.0 } );
	_position_frames = 0;
	_cues_emitted = false;

	double cumulative = 0.0;
	for ( const auto &raw: text )
	{
		// Markers are cue points, not words: they are taken out before anything is spoken, aligned or shown
		const MarkedText marked = parse_markers( raw );
		const std::string &t = marked.text;

		// Each line after the first is a place the languages are kept in step at (see Timeline.h)
		if ( !_pending_cues.empty() )
		{
			_timeline.cuts.push_back( { "\nline" + std::to_string( _pending_cues.size() ), cumulative } );
		}

		auto result = _tts->synthesize( t, _voice_ref, _tts_language );
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
		std::vector<TimedWord> line_words;
		if ( _tts->has_aligner() )
		{
			try
			{
				auto words = _tts->align( t, result.audio.value(), _tts_language );
				if ( !words.empty() )
				{
					local_start = cumulative + words.front().start_seconds;
					local_end = cumulative + words.back().end_seconds;
					line_words = match_words( t, _pending_cues.size(), cumulative, words );
				}
			}
			catch ( const std::exception & )
			{
				// Alignment unsupported/failed for this line's language -
				// keep the full-buffer fallback span computed above.
			}
		}
		// No usable alignment (unsupported language, no aligner model,
		// failure, or a word count that disagrees with split_words()):
		// spread the words proportionally over the whole line instead.
		if ( line_words.empty() )
		{
			line_words = distribute_words( t, _pending_cues.size(), cumulative, line_duration );
		}
		_words.insert( _words.end(), line_words.begin(), line_words.end() );
		for ( const auto &marker: marked.markers )
		{
			const double seconds = time_at_offset( line_words, marker.offset, cumulative );
			_markers.push_back( TimedMarker{ marker.name, seconds, _pending_cues.size() } );
			_timeline.cuts.push_back( { marker.name, std::max( marker_cut( line_words, marker.offset, seconds, cumulative ), _timeline.cuts.back().seconds ) } );
		}
		_pending_cues.push_back( PendingCue{ local_start, local_end, t } );
		cumulative += line_duration;

		_results.push_back( std::move( result ) );
	}
	_current_result_index = 0;
	_offset = 0;
	_timeline.end = cumulative;
	_duration = cumulative;

	return duration;
}

void Narrator::retime( const std::vector<double> &starts )
{
	if ( _results.empty() || starts.size() != _timeline.segments() + 1 )
	{
		return;
	}

	// The batch's audio as one buffer, as it was made (a line after another)
	size_t frames = 0;
	size_t channels = 1;
	for ( const auto &result: _results )
	{
		frames += result.audio->frames();
		channels = result.audio->channels();
	}
	AudioBuffer natural( frames, _audio_rate, channels );
	size_t at = 0;
	for ( const auto &result: _results )
	{
		const AudioBuffer &line = result.audio.value();
		if ( line.frames() > 0 )
		{
			std::memcpy( natural.frame( at ), line.data(), line.frames() * channels * sizeof( float ) );
		}
		at += line.frames();
	}

	// Where the words are in each segment: what is not a word is a pause, which is what gives first if a segment
	// has to be made shorter
	std::vector<std::vector<SpeechSpan>> speech( _timeline.segments() );
	for ( const auto &word: _words )
	{
		for ( size_t i = 0; i < _timeline.segments(); ++i )
		{
			const double from = std::max( word.start_seconds, _timeline.segment_start( i ) );
			const double to = std::min( word.end_seconds, _timeline.segment_end( i ) );
			if ( to > from )
			{
				speech[i].push_back( { from - _timeline.segment_start( i ), to - _timeline.segment_start( i ) } );
			}
		}
	}

	// ... and again with each segment where it goes, silence in what it does not fill, and any that is given less
	// time than it takes made shorter
	Retimed placed = retime_audio( natural, _timeline, starts, speech, stretch_with_ffmpeg );
	AudioBuffer out = std::move( placed.audio );
	_shortened_segments = 0;
	_largest_speedup = 1.0;
	for ( size_t i = 0; i < placed.maps.size(); ++i )
	{
		_shortened_segments += placed.maps[i].identity() ? 0 : 1;
		_largest_speedup = std::max( _largest_speedup, placed.tempos[i] );
	}

	// Everything with a time moves the same way
	for ( auto &word: _words )
	{
		word.start_seconds = retimed( _timeline, starts, placed.maps, word.start_seconds );
		word.end_seconds = retimed( _timeline, starts, placed.maps, word.end_seconds );
	}
	for ( auto &marker: _markers )
	{
		marker.seconds = retimed( _timeline, starts, placed.maps, marker.seconds );
	}
	for ( auto &cue: _pending_cues )
	{
		cue.local_start = retimed( _timeline, starts, placed.maps, cue.local_start );
		cue.local_end = retimed( _timeline, starts, placed.maps, cue.local_end );
	}
	for ( size_t i = 0; i < _timeline.cuts.size(); ++i )
	{
		_timeline.cuts[i].seconds = starts[i];
	}
	_timeline.end = starts.back();
	_duration = starts.back();

	tts::SynthesisResult combined;
	combined.success = true;
	combined.status = "ok";
	combined.audio = std::move( out );
	_results.clear();
	_results.push_back( std::move( combined ) );
	_current_result_index = 0;
	_offset = 0;
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
	_position_frames += recorded;
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

