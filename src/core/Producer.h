
#pragma once

#include "Director.h"
#include "PluginLoader.h"
#include "Screenplay.h"
#include "Stage.h"
#include "Script.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class Producer
{
public:
	// Which scene(s) to render is chosen on the command line (--scene, repeatable) from those registered
	// with REGISTER_SCENE (see SceneRegistry.h), in the program or in a plugin (--plugin, see Plugin.h); with
	// no --scene it is the scene called "movie". A Director given here is used instead of "movie" when there is
	// no --scene.
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

	// Play the scene(s) live in a window, with sound, for one language, writing nothing. The narration is
	// synthesised first (silently, so it is in the cache), then it plays in real time. What has been played is
	// kept in memory (see FrameStore.h), so it can be played backwards and stepped through:
	//   space: play/pause   L: play   J: play backwards   K: pause   left/right: one frame back/forward (with a little sound)
	//   up/down: 1 second forward/back (keeps playing)   home/end: the start/the end   escape: quit
	//   [ and ]: the previous scene (or the start of this one, if a second or more in) and the next
	//   r: repeat the scene being watched (again to stop), in either direction
	//   F5: load the plugins and the script again, and play from the scene being watched
	// Going somewhere the producer has not got to yet waits for it. A plugin (--plugin) that is rebuilt is loaded
	// again by itself, as F5 does (but not the script).
	bool preview( const std::string &language );

	// The frame rate that suits the main display, for --preview without --fps: its refresh rate, or the nearest
	// below that the audio can be divided by (audio_rate must be a whole number of samples for each frame).
	// 0 if it cannot be found.
	int display_frame_rate( size_t audio_rate );

	bool tick()
	{
		++_current_frame;

		if ( !_screenplay.done() )
		{
			_screenplay.resume();
		}

		// What is tied to something else, made so as the scene left it this frame
		_stage.updaters.run();

		return !_screenplay.done();
	}

	// First, so they go last: the scenes, and all they made, have to go before the code that made them
	std::vector<Plugin> _plugins;

	Director _director;
	Screenplay _screenplay;
	Stage _stage = {};
	std::optional<Theme> _loaded_theme; // one from a file, which the stage points at
	Script _script = {};
	int64_t _current_frame = -1;
};

