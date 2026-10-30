
#include "Narration.h"

#include <algorithm>
#include <audio/Resample.h>
#include <core/Director.h>
#include <core/Stage.h>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>

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
	_max_speedup = stage.max_speedup();
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

Coroutine Narration::animate_words( Group &text, WordEffect effect, size_t first_word )
{
	const Narrator *n = master();
	if ( !n )
	{
		co_return;
	}

	// One coroutine per word. Each waits for its word by itself, so they can all be started now.
	const auto &spoken = n->words();
	std::vector<Coroutine> running;
	for ( size_t i = 0;; ++i )
	{
		Visual *word = find_word( text, i );
		const size_t index = first_word + i;
		if ( !word || index >= spoken.size() )
		{
			break;
		}
		running.push_back( effect( WordContext{
			text,
			*word,
			index,
			spoken[index].start_seconds,
			spoken[index].end_seconds,
			[n]() { return n->position(); },
			[n]() { return n->is_done(); } } ) );
	}

	co_await choreograph_all( std::move( running ) );
}

const std::vector<TimedMarker> &Narration::markers() const
{
	static const std::vector<TimedMarker> none;
	const Narrator *n = master();
	return n ? n->markers() : none;
}

size_t Narration::marker_count() const
{
	return markers().size();
}

std::optional<size_t> Narration::find_marker( std::string_view name ) const
{
	const auto &all = markers();
	for ( size_t i = 0; i < all.size(); ++i )
	{
		if ( all[i].name == name )
		{
			return i;
		}
	}
	return std::nullopt;
}

double Narration::marker_time( size_t index ) const
{
	const auto &all = markers();
	if ( index >= all.size() )
	{
		throw std::out_of_range( "no marker number " + std::to_string( index ) + " (the narration has " + std::to_string( all.size() ) + ")" );
	}
	return all[index].seconds;
}

double Narration::marker_time( std::string_view name ) const
{
	const auto index = find_marker( name );
	if ( !index )
	{
		throw std::out_of_range( "no marker called '" + std::string( name ) + "' in the narration" );
	}
	return markers()[*index].seconds;
}

double Narration::duration() const
{
	const auto &spoken = words();
	return spoken.empty() ? 0.0 : spoken.back().end_seconds;
}

Coroutine Narration::until_marker( size_t index )
{
	co_await until_time( marker_time( index ) );
}

Coroutine Narration::until_marker( std::string name )
{
	co_await until_time( marker_time( name ) );
}

Coroutine Narration::track_markers( size_t from, size_t to, std::function<void( float )> callback )
{
	co_await track_time( marker_time( from ), marker_time( to ), std::move( callback ) );
}

Coroutine Narration::track_markers( std::string from, std::string to, std::function<void( float )> callback )
{
	co_await track_time( marker_time( from ), marker_time( to ), std::move( callback ) );
}

Coroutine Narration::track_to_end( std::string from, std::function<void( float )> callback )
{
	co_await track_time( marker_time( from ), duration(), std::move( callback ) );
}

void Narration::check_markers() const
{
	std::map<std::string, std::vector<std::string>> names;
	for ( const auto &narrator: _narrators )
	{
		auto &list = names[narrator.language()];
		for ( const auto &marker: narrator.markers() )
		{
			list.push_back( marker.name );
		}
	}

	const std::string main_language = _script->master_language();
	for ( const auto &problem: marker_mismatches( names, main_language ) )
	{
		std::cerr << "warning: " << problem << '\n';
	}
}

void Narration::synchronise()
{
	const Narrator *main = master();
	if ( _narrators.size() < 2 || !main )
	{
		return;
	}

	std::vector<Narrator *> together;
	std::vector<const Timeline *> timelines;
	size_t main_index = 0;
	for ( auto &narrator: _narrators )
	{
		if ( same_structure( narrator.timeline(), main->timeline() ) )
		{
			if ( &narrator == main )
			{
				main_index = together.size();
			}
			together.push_back( &narrator );
			timelines.push_back( &narrator.timeline() );
		}
		else
		{
			std::cerr << "warning: " << narrator.language() << " has different lines or markers from " << main->language()
					  << ", so it is not kept in step with it\n";
		}
	}
	if ( together.size() < 2 )
	{
		return;
	}

	const auto starts = segment_starts( barrier_lengths( timelines, main_index, _max_speedup ) );
	for ( Narrator *narrator: together )
	{
		narrator->retime( starts );
		if ( narrator->shortened_segments() > 0 )
		{
			std::cout << narrator->language() << ": made shorter to keep in step with " << main->language() << " in "
					  << narrator->shortened_segments() << " of " << narrator->timeline().segments() << " parts (speech speeded up by up to "
					  << narrator->largest_speedup() << "x; pauses shortened first)\n";
		}
	}
}
