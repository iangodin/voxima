
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


const Narrator *Narration::master() const
{
	for ( const auto &n: _narrators )
	{
		if ( n.language() == _script->master_language() )
		{
			return &n;
		}
	}
	return _narrators.empty() ? nullptr : &_narrators.front();
}

const std::vector<TimedWord> &Narration::words() const
{
	static const std::vector<TimedWord> empty;
	const Narrator *n = master();
	return n ? n->words() : empty;
}

Coroutine Narration::until_time( double seconds )
{
	const Narrator *n = master();
	while ( n && !n->is_done() && n->position() < seconds )
	{
		co_yield NextFrame{};
	}
}

Coroutine Narration::until_word( size_t index )
{
	const auto &w = words();
	co_await until_time( index < w.size() ? w[index].start_seconds : 1e300 );
}

Coroutine Narration::until_word_end( size_t index )
{
	const auto &w = words();
	co_await until_time( index < w.size() ? w[index].end_seconds : 1e300 );
}

Coroutine Narration::on_words( Visual &text, std::function<void( Visual &, size_t )> callback )
{
	const size_t count = words().size();
	for ( size_t i = 0; i < count; ++i )
	{
		co_await until_word( i );
		if ( Visual *word = find_word( text, i ) )
		{
			callback( *word, i );
		}
	}
}

Coroutine Narration::track_time( double from, double to, std::function<void( float )> callback )
{
	co_await until_time( from );

	const Narrator *n = master();
	while ( n && !n->is_done() )
	{
		const float t = progress( n->position(), from, to );
		if ( t >= 1.F )
		{
			break;
		}
		callback( t );
		co_yield NextFrame{};
	}
	callback( 1.F );
}

Coroutine Narration::track_words( size_t first, size_t last, std::function<void( float )> callback )
{
	const auto &w = words();
	if ( w.empty() )
	{
		callback( 1.F );
		co_return;
	}
	first = std::min( first, w.size() - 1 );
	last = std::clamp( last, first, w.size() - 1 );
	co_await track_time( w[first].start_seconds, w[last].end_seconds, std::move( callback ) );
}
