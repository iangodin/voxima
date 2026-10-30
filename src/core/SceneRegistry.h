#pragma once

#include <chrono>

#include "Director.h"
#include "Scene.h"

#include <utils/NamedRegistered.h>

#include <string>
#include <vector>

struct Stage;
class Script;

// A scene that can be found by name: what `--scene <name>` renders, and what a movie plays. A scene is a
// coroutine taking the stage and the script, so it can do anything a Screenplay can.
//
// Register one with a line next to it:
//
//     Scene show_intro( Stage &stage, Script &script ) { ... }
//     SONIMATION_SCENE( "intro", "The title and the first formula", show_intro );
//
// (the function, or any callable, only has to convert to a Director).
//
// A movie is just a scene that plays others:
//
//     Scene movie( Stage &stage, Script &script )
//     {
//         co_await sequence( stage, script, { "intro", "morphs", "outro" } );
//     }
//     SONIMATION_SCENE( "movie", "The whole thing", movie );
//
// The scene called "movie" is what renders when no --scene is given.
class SceneEntry : public NamedRegistered<SceneEntry>
{
public:
	SceneEntry( std::string name, std::string description, Director run ) :
		NamedRegistered( std::move( name ) ),
		_description( std::move( description ) ),
		_run( std::move( run ) )
	{
	}

	const std::string &description() const
	{
		return _description;
	}

	Scene run( Stage &stage, Script &script ) const
	{
		return _run( stage, script );
	}

	// Every registered scene, by name
	static std::vector<const SceneEntry *> all();

private:
	std::string _description;
	Director _run;
};

// The one registry of scenes, in the core library (see NamedRegistered.h)
template<>
std::unordered_map<std::string, SceneEntry *> &NamedRegistered<SceneEntry>::get_registry();

#define SONIMATION_SCENE_CONCAT_( a, b ) a##b
#define SONIMATION_SCENE_CONCAT( a, b ) SONIMATION_SCENE_CONCAT_( a, b )
#define SONIMATION_SCENE( name, description, function ) \
	static const SceneEntry SONIMATION_SCENE_CONCAT( sonimation_scene_, __LINE__ )( name, description, function )

struct PlayOptions
{
	// Start the scene on an empty stage, so it looks the same on its own as in a movie. Turn it off for a
	// scene that carries on from what the one before left.
	bool clear_stage = true;

	// A transition between scenes: what was on the stage fades out before it is cleared (the new scene starts when it has gone),
	// and the new scene fades in over its first moments, as it goes on. Both 0 is a cut, as it has been.
	std::chrono::duration<double> fade_out{ 0.0 };
	std::chrono::duration<double> fade_in{ 0.0 };
};

// Play the registered scene called `name`, and finish when it does. An unknown name is a mistake in the
// program (the command line's names are checked beforehand): it says so and throws.
Scene play( Stage &stage, Script &script, std::string name, PlayOptions options = {} );

// Play scenes one after another
Scene sequence( Stage &stage, Script &script, std::vector<std::string> names, PlayOptions options = {} );

// Which of these names are not registered scenes
std::vector<std::string> unknown_scenes( const std::vector<std::string> &names );

// The registered scenes, one per line with their descriptions, for --list-scenes and error messages
std::string describe_scenes();
