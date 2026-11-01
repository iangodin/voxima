#include <catch2/catch_test_macros.hpp>

#include <core/Chapters.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>

#include <chrono>
#include <string>
#include <vector>

using namespace std::chrono_literals;

namespace
{
std::vector<std::string> names_of( const std::vector<Chapter> &chapters )
{
	std::vector<std::string> names;
	for ( const Chapter &chapter: chapters )
	{
		names.push_back( chapter.name );
	}
	return names;
}

PlayedScene played( std::string name, int depth, int64_t start, int64_t end, std::string chapter = {} )
{
	return { std::move( name ), std::move( chapter ), depth, start, end };
}

Scene chapter_test_scene( Stage &stage, Script & )
{
	co_await wait_for( stage, 0.1s );
}
REGISTER_SCENE( "chapter-test", "A scene for the test of chapters", chapter_test_scene );
}

TEST_CASE( "The scenes played one after another are the chapters", "[chapters]" )
{
	const auto chapters = choose_chapters( { played( "a", 0, 0, 10 ), played( "b", 0, 10, 30 ), played( "c", 0, 30, 40 ) }, 40 );
	CHECK( names_of( chapters ) == std::vector<std::string>{ "a", "b", "c" } );
	CHECK( chapters[1].start == 10 );
	CHECK( chapters[1].end == 30 );
	CHECK( chapters[2].end == 40 );
}

TEST_CASE( "A movie's top-level scenes are the chapters", "[chapters]" )
{
	// movie -> parts -> sections
	const auto chapters = choose_chapters(
		{ played( "movie", 0, 0, 40 ),
		  played( "part1", 1, 0, 20 ),
		  played( "s1", 2, 0, 10 ),
		  played( "s2", 2, 10, 20 ),
		  played( "part2", 1, 20, 40 ),
		  played( "s3", 2, 20, 40 ) },
		40 );
	CHECK( names_of( chapters ) == std::vector<std::string>{ "part1", "part2" } );
}

TEST_CASE( "Marked scenes are the only chapters, at any depth, with their titles", "[chapters]" )
{
	const auto chapters = choose_chapters(
		{ played( "movie", 0, 0, 40 ), played( "part1", 1, 0, 20 ), played( "s2", 2, 10, 20, "The middle" ), played( "part2", 1, 20, 40, "The end" ) }, 40 );
	CHECK( names_of( chapters ) == std::vector<std::string>{ "s2", "part2" } );
	CHECK( chapters[0].title == "The middle" );
	CHECK( chapters[0].start == 0 ); // the first starts at the start: players expect it
	CHECK( chapters[1].start == 20 );
}

TEST_CASE( "One scene, or none, makes no chapters", "[chapters]" )
{
	CHECK( choose_chapters( {}, 10 ).empty() );
	CHECK( choose_chapters( { played( "a", 0, 0, 10 ) }, 10 ).empty() );
	CHECK( choose_chapters( { played( "movie", 0, 0, 10 ), played( "a", 1, 0, 10 ) }, 10 ).empty() );
}

TEST_CASE( "Scenes of no length are left out", "[chapters]" )
{
	// (what a --start-at went past)
	const auto chapters = choose_chapters( { played( "a", 0, 0, 0 ), played( "b", 0, 0, 10 ), played( "c", 0, 10, 20 ) }, 20 );
	CHECK( names_of( chapters ) == std::vector<std::string>{ "b", "c" } );
}

TEST_CASE( "A scene still running at the end counts", "[chapters]" )
{
	const auto chapters = choose_chapters( { played( "a", 0, 0, 10 ), played( "b", 0, 10, -1 ) }, 25 );
	REQUIRE( chapters.size() == 2 );
	CHECK( chapters[1].end == 25 );
}

TEST_CASE( "Chapters are written for ffmpeg, in milliseconds, with its characters escaped", "[chapters]" )
{
	const std::string text = chapter_metadata( { { "a", "One = #1; \\ two", 0, 12 }, { "b", "Two", 12, 36 } }, 24.0 );
	CHECK( text ==
		";FFMETADATA1\n"
		"[CHAPTER]\nTIMEBASE=1/1000\nSTART=0\nEND=500\ntitle=One \\= \\#1\\; \\\\ two\n"
		"[CHAPTER]\nTIMEBASE=1/1000\nSTART=500\nEND=1500\ntitle=Two\n" );
}

TEST_CASE( "PlayOptions::chapter tells the stage", "[chapters][scenes]" )
{
	Stage stage;
	Script script;
	std::vector<std::string> told;
	stage.scene_event = [&]( const std::string &name, bool started )
	{
		told.push_back( name + ( started ? " starts" : " ends" ) );
	};
	stage.chapter_event = [&]( const std::string &title ) { told.push_back( "chapter " + title ); };

	PlayOptions options;
	options.chapter = "The title";
	Scene scene = play( stage, script, "chapter-test", options );
	while ( !scene.done() )
	{
		scene.resume();
	}
	CHECK( told == std::vector<std::string>{ "chapter The title", "chapter-test starts", "chapter-test ends" } );

	told.clear();
	Scene plain = play( stage, script, "chapter-test" );
	while ( !plain.done() )
	{
		plain.resume();
	}
	CHECK( told == std::vector<std::string>{ "chapter-test starts", "chapter-test ends" } );
}
