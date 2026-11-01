#include "PlayRange.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

PlayPoint PlayPoint::parse( const std::string &text )
{
	PlayPoint point;
	if ( !text.empty() && std::all_of( text.begin(), text.end(), []( unsigned char c ) { return std::isdigit( c ) != 0; } ) )
	{
		try
		{
			point.frame = std::stoll( text );
		}
		catch ( const std::out_of_range & )
		{
			throw std::runtime_error( "frame number " + text + " is too large" );
		}
	}
	else
	{
		point.scene = text;
	}
	return point;
}

std::string PlayPoint::describe() const
{
	return frame ? "frame " + std::to_string( *frame ) : "scene " + scene;
}

PlayRange::PlayRange( std::optional<PlayPoint> start, std::optional<PlayPoint> stop ) :
	_start( std::move( start ) ),
	_stop( std::move( stop ) ),
	_started( !_start )
{
	if ( _start && _stop && _start->frame && _stop->frame && *_stop->frame < *_start->frame )
	{
		throw std::runtime_error( "--stop-at is before --start-at" );
	}
}

void PlayRange::scene_event( const std::string &name, bool started )
{
	if ( started )
	{
		if ( !_started && _start->scene == name )
		{
			_started = true;
		}
	}
	else if ( _started && _stop && _stop->scene == name )
	{
		_stopping = true;
	}
}

void PlayRange::frame_made( int64_t frame )
{
	if ( !_started && _start->frame && frame >= *_start->frame )
	{
		_started = true;
	}
	if ( _stop && _stop->frame && frame > *_stop->frame )
	{
		_stopping = true;
	}
}
