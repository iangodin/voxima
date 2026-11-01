#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <audio/SubtitleCue.h>
#include <core/PlayRange.h>

#include <stdexcept>
#include <vector>

using Catch::Approx;

TEST_CASE( "A place is a frame if it is all digits, and otherwise a scene", "[playrange]" )
{
	CHECK( PlayPoint::parse( "120" ).frame == 120 );
	CHECK( PlayPoint::parse( "0" ).frame == 0 );
	CHECK_FALSE( PlayPoint::parse( "intro" ).frame );
	CHECK( PlayPoint::parse( "intro" ).scene == "intro" );
	CHECK( PlayPoint::parse( "part2" ).scene == "part2" );
	CHECK( PlayPoint::parse( "2-part" ).scene == "2-part" );
	CHECK( PlayPoint::parse( "-5" ).scene == "-5" );
	CHECK( PlayPoint::parse( "120" ).describe() == "frame 120" );
	CHECK( PlayPoint::parse( "intro" ).describe() == "scene intro" );
	CHECK_THROWS_AS( PlayPoint::parse( "99999999999999999999999" ), std::runtime_error );
}

TEST_CASE( "With no range, everything is wanted", "[playrange]" )
{
	PlayRange range;
	CHECK_FALSE( range.skipping() );
	range.scene_event( "a", true );
	range.frame_made( 1000 );
	range.scene_event( "a", false );
	CHECK_FALSE( range.skipping() );
	CHECK_FALSE( range.stopping() );
}

TEST_CASE( "A range starts at a frame, and stops after one", "[playrange]" )
{
	PlayRange range( PlayPoint::parse( "10" ), PlayPoint::parse( "12" ) );
	CHECK( range.skipping() );
	range.frame_made( 9 );
	CHECK( range.skipping() );
	range.frame_made( 10 ); // the first frame wanted
	CHECK_FALSE( range.skipping() );
	range.frame_made( 12 ); // the last one
	CHECK_FALSE( range.stopping() );
	range.frame_made( 13 );
	CHECK( range.stopping() );
}

TEST_CASE( "A range starts where a scene starts, and stops where one ends", "[playrange]" )
{
	PlayRange range( PlayPoint::parse( "b" ), PlayPoint::parse( "c" ) );
	range.scene_event( "a", true );
	range.scene_event( "a", false );
	CHECK( range.skipping() );
	range.scene_event( "b", true );
	CHECK_FALSE( range.skipping() );
	range.scene_event( "b", false );
	range.scene_event( "c", true );
	CHECK_FALSE( range.stopping() );
	range.scene_event( "c", false );
	CHECK( range.stopping() );
}

TEST_CASE( "One scene can be the start and the stop", "[playrange]" )
{
	PlayRange range( PlayPoint::parse( "b" ), PlayPoint::parse( "b" ) );
	range.scene_event( "b", true );
	CHECK_FALSE( range.skipping() );
	CHECK_FALSE( range.stopping() );
	range.scene_event( "b", false );
	CHECK( range.stopping() );
}

TEST_CASE( "A stop before the start does not count", "[playrange]" )
{
	PlayRange range( PlayPoint::parse( "b" ), PlayPoint::parse( "a" ) );
	range.scene_event( "a", true );
	range.scene_event( "a", false );
	CHECK_FALSE( range.stopping() );
	CHECK( range.skipping() );

	CHECK_THROWS_WITH( PlayRange( PlayPoint::parse( "20" ), PlayPoint::parse( "10" ) ), "--stop-at is before --start-at" );
}

TEST_CASE( "A scene and a frame can be mixed", "[playrange]" )
{
	PlayRange range( PlayPoint::parse( "b" ), PlayPoint::parse( "100" ) );
	range.frame_made( 50 );
	range.scene_event( "b", true );
	range.frame_made( 51 );
	CHECK_FALSE( range.skipping() );
	range.frame_made( 101 );
	CHECK( range.stopping() );
}

TEST_CASE( "Subtitles are cut to the part of the movie that was kept", "[playrange]" )
{
	const std::vector<SubtitleCue> cues{ { 1.0, 2.0, "before" }, { 2.5, 3.5, "across the start" }, { 4.0, 5.0, "inside" }, { 5.5, 6.5, "across the end" }, { 7.0, 8.0, "after" } };
	const auto kept = cues_between( cues, 3.0, 6.0 );
	REQUIRE( kept.size() == 3 );
	CHECK( kept[0].text == "across the start" );
	CHECK( kept[0].start_seconds == Approx( 0.0 ) );
	CHECK( kept[0].end_seconds == Approx( 0.5 ) );
	CHECK( kept[1].start_seconds == Approx( 1.0 ) );
	CHECK( kept[1].end_seconds == Approx( 2.0 ) );
	CHECK( kept[2].text == "across the end" );
	CHECK( kept[2].start_seconds == Approx( 2.5 ) );
	CHECK( kept[2].end_seconds == Approx( 3.0 ) );

	// A cue that ends exactly where the part starts is not in it
	CHECK( cues_between( { { 1.0, 3.0, "x" } }, 3.0, 6.0 ).empty() );
	CHECK( cues_between( { { 6.0, 7.0, "x" } }, 3.0, 6.0 ).empty() );
}
