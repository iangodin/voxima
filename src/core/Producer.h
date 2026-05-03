
#pragma once

#include "Director.h"
#include "Screenplay.h"
#include "Stage.h"
#include "Script.h"

#include <cstdint>

class Producer
{
public:
	// We give the Producer the top-level Screenplay to run
	explicit Producer( Director director ) :
		_director{ std::move( director ) }
	{
	}

	int64_t current_frame( void ) const
	{
		return _current_frame;
	}

	bool produce( int argc, char *argv[] );

private:
	bool tick()
	{
		++_current_frame;

		if ( !_screenplay.done() )
		{
			_screenplay.resume();
		}

		return !_screenplay.done();
	}

	Director _director;
	Screenplay _screenplay;
	Stage _stage = {};
	Script _script = {};
	int64_t _current_frame = -1;
};

