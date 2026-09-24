
#include "DubbingStage.h"

void DubbingStage::setup(
	const std::filesystem::path &render_dir,
	std::string_view codec,
	size_t audio_rate,
	size_t video_rate,
	const std::filesystem::path &tts_model,
	std::filesystem::path tts_voice_dir,
	const std::string &tts_backend,
	int tts_threads )
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

	// Loads the model and creates a session immediately - a missing/bad
	// model file fails here, at startup, rather than mid-render.
	_tts.emplace( tts_model, tts_backend, tts_threads );
	_tts->setup( _render_dir );
}

void DubbingStage::add_language( std::string_view lang )
{
	AudioMixer &mixer = _mixers[std::string( lang )];
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

void DubbingStage::write_audio( void )
{
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
