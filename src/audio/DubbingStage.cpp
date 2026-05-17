
#include "DubbingStage.h"

void DubbingStage::setup(
	const std::filesystem::path &render_dir, size_t audio_rate, size_t video_rate )
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

	_tts.setup( _render_dir );
}

void DubbingStage::add_language( std::string_view lang )
{
	AudioMixer &mixer = _mixers[std::string( lang )];
	auto audio_file = _render_dir / ( "output." + std::string( lang ) + ".flac" );
	mixer.open( audio_file, _audio_rate, _block_size );

	_mics[std::string( lang )] = Microphone( mixer );
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
