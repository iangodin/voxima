// All the examples as one movie: the scenes of the other examples, one after another, each on a fresh stage, with
// music under them (if VOXIMA_MUSIC names a file) that fades in at the start and out at the end.
//
// It plays scenes of the other plugins by name, so they have to be loaded with it: with Voxima's build,
//
//     voxima --plugin-dir build/Release/plugins --script-dir examples/tour/scripts --lang en --preview
//
// (A scene called "movie" is what plays without --scene.) The narrated scenes are left out: they need the speech
// models, and a narrator's voice for the language (see the narration example).

#include <core/Animate.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>

using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
// --8<-- [start:movie]
Scene movie( Stage &stage, Script &script )
{
	Sound music;
	if ( const char *file = std::getenv( "VOXIMA_MUSIC" ); file != nullptr && std::filesystem::exists( file ) )
	{
		music = stage.audio.play( file, { .volume = -22.0, .loop = true, .fade_in = 2s } );
	}

	co_await sequence(
		stage,
		script,
		{ "hello", "coroutine-basics",
		  "layout-basics", "layout", "tables",
		  "text-basics", "styled-text", "formulas", "math-marks", "code",
		  "verbs", "camera",
		  "morph-basics", "morph-shapes", "morph-formula", "morph-text", "morph-strokes",
		  "plot", "plot-edge", "charts", "diagram", "geometry",
		  "notes" },
		{ .fade_out = 0.4s, .fade_in = 0.4s } );

	music.fade_out( 3s );
	co_await wait_for( stage, 3s );
}
// --8<-- [end:movie]
}

REGISTER_SCENE( "movie", "Every example's scenes, one after another (without the narrated ones)", movie );
