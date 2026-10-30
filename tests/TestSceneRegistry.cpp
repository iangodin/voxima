#include <catch2/catch_test_macros.hpp>

#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>
#include <visuals/Circle.h>

#include <algorithm>
#include <string>
#include <vector>

namespace
{
// A scene that notes what the stage held when it started, adds a visual, lasts a few frames, and notes its end
Scene counting_scene( std::vector<std::string> &log, std::string name, Stage &stage, int frames )
{
	log.push_back( name + " starts with " + std::to_string( stage.visuals.size() ) + " visuals" );
	stage.visuals.add( std::make_unique<Circle>() );
	for ( int i = 0; i < frames; ++i )
	{
		co_await next_frame();
	}
	log.push_back( name + " ends" );
}

Scene macro_scene( Stage &stage, Script & )
{
	stage.visuals.add( std::make_unique<Circle>() );
	co_await next_frame();
}

// Run a scene to its end, the way the producer does a frame at a time
int run( Scene &scene )
{
	int frames = 0;
	while ( !scene.done() && frames < 1000 )
	{
		scene.resume();
		++frames;
	}
	return frames;
}

bool contains( const std::vector<std::string> &log, const std::string &line )
{
	return std::find( log.begin(), log.end(), line ) != log.end();
}
}

SONIMATION_SCENE( "test-macro", "Registered with the macro", macro_scene );

TEST_CASE( "SONIMATION_SCENE registers a scene by name", "[scenes]" )
{
	const SceneEntry *entry = SceneEntry::get_by_name( "test-macro" );
	REQUIRE( entry != nullptr );
	CHECK( entry->get_name() == "test-macro" );
	CHECK( entry->description() == "Registered with the macro" );

	Stage stage;
	Script script;
	Scene scene = entry->run( stage, script );
	run( scene );
	CHECK( stage.visuals.size() == 1 );
}

TEST_CASE( "SceneEntry lists scenes by name and forgets them when they go", "[scenes]" )
{
	std::vector<std::string> log;
	{
		SceneEntry b( "test-b", "second", [&]( Stage &stage, Script & ) { return counting_scene( log, "b", stage, 1 ); } );
		SceneEntry a( "test-a", "first", [&]( Stage &stage, Script & ) { return counting_scene( log, "a", stage, 1 ); } );

		REQUIRE( SceneEntry::get_by_name( "test-a" ) == &a );
		CHECK( SceneEntry::get_by_name( "test-nothing" ) == nullptr );

		// sorted, whatever the order they were registered in
		std::vector<std::string> names;
		for ( const auto *entry: SceneEntry::all() )
		{
			names.push_back( entry->get_name() );
		}
		CHECK( std::is_sorted( names.begin(), names.end() ) );
		CHECK( std::count( names.begin(), names.end(), "test-a" ) == 1 );
		CHECK( std::count( names.begin(), names.end(), "test-b" ) == 1 );

		const std::string text = describe_scenes();
		CHECK( text.find( "test-a" ) != std::string::npos );
		CHECK( text.find( "first" ) != std::string::npos );
		CHECK( text.find( "second" ) != std::string::npos );
	}
	CHECK( SceneEntry::get_by_name( "test-a" ) == nullptr );
	CHECK( SceneEntry::get_by_name( "test-b" ) == nullptr );
}

TEST_CASE( "unknown_scenes names the ones that are not registered", "[scenes]" )
{
	std::vector<std::string> log;
	SceneEntry a( "test-a", "", [&]( Stage &stage, Script & ) { return counting_scene( log, "a", stage, 1 ); } );

	CHECK( unknown_scenes( { "test-a" } ).empty() );
	CHECK( unknown_scenes( {} ).empty() );
	CHECK( unknown_scenes( { "test-a", "nope", "test-a", "also-nope" } ) == std::vector<std::string>{ "nope", "also-nope" } );
}

TEST_CASE( "play runs a registered scene on an empty stage", "[scenes]" )
{
	std::vector<std::string> log;
	SceneEntry a( "test-a", "", [&]( Stage &stage, Script & ) { return counting_scene( log, "a", stage, 3 ); } );

	Stage stage;
	Script script;
	stage.visuals.add( std::make_unique<Circle>() ); // left over from something else

	Scene scene = play( stage, script, "test-a" );
	const int frames = run( scene );

	CHECK( scene.done() );
	CHECK( frames >= 3 );
	CHECK( log == std::vector<std::string>{ "a starts with 0 visuals", "a ends" } );

	SECTION( "unless it is asked not to" )
	{
		log.clear();
		Stage other;
		other.visuals.add( std::make_unique<Circle>() );
		PlayOptions keep;
		keep.clear_stage = false;
		Scene again = play( other, script, "test-a", keep );
		run( again );
		CHECK( contains( log, "a starts with 1 visuals" ) );
	}
}

TEST_CASE( "sequence plays scenes one after another, each on a fresh stage", "[scenes]" )
{
	std::vector<std::string> log;
	SceneEntry a( "test-a", "", [&]( Stage &stage, Script & ) { return counting_scene( log, "a", stage, 2 ); } );
	SceneEntry b( "test-b", "", [&]( Stage &stage, Script & ) { return counting_scene( log, "b", stage, 1 ); } );

	Stage stage;
	Script script;

	SECTION( "in the order given, and repeated ones play again" )
	{
		Scene scene = sequence( stage, script, { "test-a", "test-b", "test-a" } );
		run( scene );
		CHECK( scene.done() );
		CHECK( log ==
			   std::vector<std::string>{ "a starts with 0 visuals",
										 "a ends",
										 "b starts with 0 visuals",
										 "b ends",
										 "a starts with 0 visuals",
										 "a ends" } );
	}

	SECTION( "the stage is only cleared before a scene, so the last one stays on it" )
	{
		Scene scene = sequence( stage, script, { "test-a", "test-b" } );
		run( scene );
		CHECK( stage.visuals.size() == 1 );
	}

	SECTION( "without clearing, each scene sees what the one before left" )
	{
		PlayOptions keep;
		keep.clear_stage = false;
		Scene scene = sequence( stage, script, { "test-a", "test-b" }, keep );
		run( scene );
		CHECK( contains( log, "b starts with 1 visuals" ) );
		CHECK( stage.visuals.size() == 2 );
	}

	SECTION( "an empty sequence finishes at once" )
	{
		Scene scene = sequence( stage, script, {} );
		run( scene );
		CHECK( scene.done() );
		CHECK( log.empty() );
	}
}

TEST_CASE( "a movie is a scene that plays other scenes", "[scenes]" )
{
	std::vector<std::string> log;
	SceneEntry a( "test-a", "", [&]( Stage &stage, Script & ) { return counting_scene( log, "a", stage, 1 ); } );
	SceneEntry b( "test-b", "", [&]( Stage &stage, Script & ) { return counting_scene( log, "b", stage, 1 ); } );
	SceneEntry movie(
		"test-movie",
		"",
		[]( Stage &stage, Script &script ) -> Scene { return sequence( stage, script, { "test-b", "test-a" } ); } );

	Stage stage;
	Script script;
	Scene scene = play( stage, script, "test-movie" );
	run( scene );

	CHECK( scene.done() );
	CHECK( log == std::vector<std::string>{ "b starts with 0 visuals", "b ends", "a starts with 0 visuals", "a ends" } );
}

namespace
{
Scene long_scene( Stage &stage, Script & )
{
	stage.visuals.add( std::make_unique<Circle>() );
	for ( int i = 0; i < 30; ++i )
	{
		co_await next_frame();
	}
}
}

SONIMATION_SCENE( "test-long", "A scene of thirty frames", long_scene );

TEST_CASE( "play fades the old scene out and the new one in", "[scenes]" )
{
	using namespace std::chrono_literals;
	Stage stage; // (24 frames a second)
	Script script;

	Scene scene = sequence( stage, script, { "test-long", "test-long" }, { .fade_out = 0.5s, .fade_in = 0.5s } );
	float lowest = 1.F;
	float first = -1.F;
	int frames = 0;
	while ( !scene.done() && frames < 1000 )
	{
		scene.resume();
		stage.updaters.run();
		if ( first < 0.F )
		{
			first = stage.visuals.color.a;
		}
		lowest = std::min( lowest, stage.visuals.color.a );
		++frames;
	}
	CHECK( first < 0.2F );                    // the first scene comes in from nothing
	CHECK( lowest < 0.1F );
	CHECK( stage.visuals.color.a == 1.F );    // and it ends at full
	CHECK( stage.updaters.size() == 0 );
}
