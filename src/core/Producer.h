
#pragma once

#include "Director.h"
#include "Screenplay.h"
#include "Stage.h"
#include "Script.h"

#include <cstdint>
#include <string>
#include <vector>

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
	// One render: the languages that go into one container, where it and its files are written, and how
	struct RenderJob
	{
		std::vector<std::string> languages; // the first is the master
		std::string render_dir;
		std::string container;
		std::string vcodec;
		std::string acodec;
		std::string scodec;
		std::string subtitle_ext;
		bool save_png = false;
		bool save_svg = false;
	};

	// Play the scene(s) through once for these languages, and write the container. The models are loaded (by
	// produce()) once for all the renders.
	bool render( const RenderJob &job );

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

