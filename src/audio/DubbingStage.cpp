
#include "DubbingStage.h"

#include "Resample.h"
#include "SoundFile.h"

#include <algorithm>
#include <stdexcept>
#include <system_error>

void DubbingStage::setup(
	const std::filesystem::path &render_dir,
	std::string_view codec,
	size_t audio_rate,
	size_t video_rate,
	const std::filesystem::path &tts_model,
	std::filesystem::path tts_voice_dir,
	const std::string &tts_backend,
	int tts_threads,
	const std::filesystem::path &aligner_model )
{
	if ( audio_rate % video_rate != 0 )
	{
		throw std::runtime_error(
			"audio rate " + std::to_string( audio_rate ) + " not divisible by video rate " +
			std::to_string( video_rate ) );
	}

	_render_dir = render_dir;
	_audio_rate = audio_rate;
	_block_size = audio_rate / video_rate;
	_codec = codec;
	_tts_voice_dir = std::move( tts_voice_dir );

	_tts_dir = render_dir; // where it keeps what it made, though begin() may render elsewhere
	_tts_model = tts_model;
	_tts_backend = tts_backend;
	_tts_threads = tts_threads;
	_aligner_model = aligner_model;

	// Loads the model and creates a session immediately - a bad model file
	// fails here, at startup, rather than mid-render. With no model file at
	// all, there may be no narration to need one (a first try of an installed
	// Voxima, say): it waits to be asked for, and says what is missing
	// then. The aligner (if any) loads lazily on first use inside
	// TTSClient::align().
	std::error_code error;
	if ( std::filesystem::exists( _tts_model, error ) )
	{
		tts();
	}
}

tts::TTSClient &DubbingStage::tts()
{
	if ( !_tts )
	{
		std::error_code error;
		if ( !std::filesystem::exists( _tts_model, error ) )
		{
			throw std::runtime_error(
				"narration needs a TTS model, and there is none at " + _tts_model.string() +
				" (give one with --tts-model or $VOXIMA_MODELS; see download_models.py)" );
		}
		_tts.emplace( _tts_model, _tts_backend, _tts_threads, _aligner_model );
		_tts->setup( _tts_dir );
	}
	return *_tts;
}

void DubbingStage::begin( const std::filesystem::path &render_dir, const std::vector<std::string> &languages, bool in_memory )
{
	_mixers.clear();
	_mics.clear();
	_playing.clear();
	_outputs.clear();
	_subtitles.clear();

	_render_dir = render_dir;
	std::filesystem::create_directories( _render_dir );
	for ( const auto &lang: languages )
	{
		add_language( lang, in_memory );
	}
}

std::vector<float> DubbingStage::take_audio( std::string_view lang )
{
	auto m = _mixers.find( lang );
	if ( m == _mixers.end() )
	{
		throw std::runtime_error( "unable to find audio for language " + std::string( lang ) );
	}
	return m->second.take_memory();
}

void DubbingStage::add_language( std::string_view lang, bool in_memory )
{
	AudioMixer &mixer = _mixers[std::string( lang )];
	if ( in_memory )
	{
		mixer.open_memory( _audio_rate, _block_size );
		_mics[std::string( lang )] = Microphone( mixer );
		return;
	}
	auto audio_file = _render_dir / ( "audio." + std::string( lang ) + "." + _codec );
	mixer.open( audio_file.string(), _audio_rate, _block_size );

	_mics[std::string( lang )] = Microphone( mixer );
	_outputs[std::string( lang )] = audio_file.string();
}

void DubbingStage::add_audio( const AudioBuffer &buffer )
{
	for ( auto &[lang, mixer]: _mixers )
	{
		mixer.add_audio( buffer );
	}
}

Microphone &DubbingStage::mic( std::string_view lang )
{
	auto m = _mics.find( lang );
	if ( m == _mics.end() )
	{
		throw std::runtime_error( "unable to find mic for language " + std::string( lang ) );
	}

	return m->second;
}

const std::string &DubbingStage::output( std::string_view lang )
{
	auto o = _outputs.find( lang );
	if ( o == _outputs.end() )
	{
		throw std::runtime_error( "unable to find audio outpout for language " + std::string( lang ) );
	}

	return o->second;
}

void DubbingStage::caption( std::string_view lang, SubtitleCue cue )
{
	_subtitles[std::string( lang )].push_back( std::move( cue ) );
}

const std::vector<SubtitleCue> &DubbingStage::subtitles( std::string_view lang ) const
{
	static const std::vector<SubtitleCue> empty;
	auto s = _subtitles.find( lang );
	return s == _subtitles.end() ? empty : s->second;
}

Sound DubbingStage::play( const std::filesystem::path &file, const SoundOptions &options )
{
	const std::string key = std::filesystem::absolute( file ).lexically_normal().string();
	auto found = _sound_files.find( key );
	if ( found == _sound_files.end() )
	{
		found = _sound_files.emplace( key, std::make_shared<const AudioBuffer>( load_sound( file, _audio_rate ) ) ).first;
	}
	return start( found->second, options );
}

Sound DubbingStage::play( const AudioBuffer &audio, const SoundOptions &options )
{
	return start( std::make_shared<const AudioBuffer>( resample( to_stereo( audio ), _audio_rate ) ), options );
}

Sound DubbingStage::start( std::shared_ptr<const AudioBuffer> audio, const SoundOptions &options )
{
	auto playing = std::make_shared<PlayingSound>( std::move( audio ), options );
	_playing.push_back( playing );
	return Sound( std::move( playing ) );
}

void DubbingStage::stop_sounds()
{
	for ( auto &playing: _playing )
	{
		playing->stop();
	}
	_playing.clear();
}

void DubbingStage::write_audio( void )
{
	// The sounds' next block, the same in every language
	if ( !_playing.empty() )
	{
		if ( !_sound_block.same( AudioBuffer( _block_size, _audio_rate, 2 ) ) )
		{
			_sound_block = AudioBuffer( _block_size, _audio_rate, 2 );
		}
		std::fill( _sound_block.begin(), _sound_block.end(), 0.F );
		for ( auto &playing: _playing )
		{
			playing->mix_into( _sound_block );
		}
		std::erase_if( _playing, []( const auto &playing ) { return playing->done(); } );

		for ( auto &[lang, mixer]: _mixers )
		{
			mixer.add_audio( _sound_block );
		}
	}

	for ( auto &[lang, mixer]: _mixers )
	{
		mixer.write_audio();
	}
}

void DubbingStage::close()
{
	for ( auto &[lang, mixer]: _mixers )
	{
		mixer.close();
	}
}
