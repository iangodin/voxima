#include <catch2/catch_test_macros.hpp>

#include <core/Camera.h>
#include <core/Stage.h>
#include <core/Wait.h>

#include <chrono>

using namespace std::chrono_literals;

namespace
{
// How many frames a scene that only waits takes to finish: one for each resume that leaves it waiting
int64_t frames_taken( Coroutine scene )
{
	int64_t frames = 0;
	scene.resume();
	while ( !scene.done() )
	{
		++frames;
		scene.resume();
	}
	return frames;
}
} // namespace

TEST_CASE( "Camera::frames turns a time into whole frames at its frame rate", "[wait]" )
{
	Camera camera;

	camera.frame_rate = 24;
	CHECK( camera.frames( 1s ) == 24 );
	CHECK( camera.frames( 0.5s ) == 12 );
	CHECK( camera.frames( 1500ms ) == 36 );
	CHECK( camera.frames( 0s ) == 0 );

	camera.frame_rate = 60;
	CHECK( camera.frames( 1s ) == 60 );
	CHECK( camera.frames( 0.5s ) == 30 );
	CHECK( camera.frames( 1.7s ) == 102 );

	// Not a whole number of frames: the nearest one (half a frame goes up)
	camera.frame_rate = 25;
	CHECK( camera.frames( 0.5s ) == 13 );
	CHECK( camera.frames( 0.45s ) == 11 );
	CHECK( camera.frames( 10ms ) == 0 );

	// Never a negative number
	CHECK( camera.frames( -1s ) == 0 );
}

TEST_CASE( "Camera::duration is the way back from frames", "[wait]" )
{
	Camera camera;
	camera.frame_rate = 50;
	CHECK( camera.duration( 25 ).count() == 0.5 );
	CHECK( camera.frames( camera.duration( 37 ) ) == 37 );
}

TEST_CASE( "wait_for lasts as long whatever the frame rate", "[wait]" )
{
	Stage stage;

	stage.camera.frame_rate = 24;
	CHECK( frames_taken( wait_for( stage, 2s ) ) == 48 );

	stage.camera.frame_rate = 60;
	CHECK( frames_taken( wait_for( stage, 2s ) ) == 120 );

	stage.camera.frame_rate = 30;
	CHECK( frames_taken( wait_for( stage, 250ms ) ) == 8 ); // 7.5 frames, to the nearest
	CHECK( frames_taken( wait_for( stage, 0s ) ) == 0 );
}

TEST_CASE( "wait_frames counts frames, not time", "[wait]" )
{
	CHECK( frames_taken( wait_frames( 12 ) ) == 12 );
	CHECK( frames_taken( wait_frames( 0 ) ) == 0 );
}
