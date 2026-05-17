
#include "Narration.h"

#include <algorithm>
#include <audio/Resample.h>
#include <core/Stage.h>
#include <cstring>

void Narration::setup( std::string_view name, Script &script, DubbingStage &stage )
{
	_script = script;
	for ( const auto &language: _script->languages() )
	{
		Narrator narrator;
		narrator.setup( name, language, script, stage );
		_narrators.push_back( std::move( narrator ) );
	}

	_audio_block = stage.audio_block();
}

void Narration::speak( void )
{
	for ( auto &narrator: _narrators )
	{
		narrator.speak( _audio_block );
	}
}

bool Narration::is_done() const
{
	auto is_done = []( const Narrator &n )
	{
		return n.is_done();
	};

	return std::all_of( _narrators.begin(), _narrators.end(), is_done );
}

