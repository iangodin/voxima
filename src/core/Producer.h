
#pragma once

#include "Director.h"
#include "Screenplay.h"
#include "Stage.h"
#include "Script.h"

#include <cstdint>

class Producer
{
public:
	// Which scene(s) to render is chosen on the command line (--scene, repeatable) from those registered
	// with SONIMATION_SCENE (see SceneRegistry.h); with no --scene it is the scene called "movie". A
	// Director given here is used instead of "movie" when there is no --scene.
	explicit Producer( Director director = {} ) :
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

