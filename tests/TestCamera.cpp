#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/CameraMove.h>
#include <core/Stage.h>
#include <visuals/Rectangle.h>

#include <chrono>
#include <cmath>
#include <sstream>

using Catch::Approx;
using namespace std::chrono_literals;

namespace
{
std::string svg_of( const Stage &stage )
{
	std::ostringstream out;
	stage.save_svg( out );
	return out.str();
}

bool contains( const std::string &text, const std::string &part )
{
	return text.find( part ) != std::string::npos;
}

// Where a stage point is on the picture, from -0.5 to 0.5 across (and up): what a viewer sees
Point on_screen( const CameraView &view, Point p )
{
	return ( p - view.center ) * view.zoom;
}

void run( Coroutine scene )
{
	scene.resume();
	while ( !scene.done() )
	{
		scene.resume();
	}
}
}

TEST_CASE( "The camera's default view is the one there always was", "[camera]" )
{
	Stage stage;
	CHECK( contains( svg_of( stage ), "viewBox=\"-8 -4.5 16 9\"" ) );

	AABB seen = stage.camera.visible();
	CHECK( seen.min.x() == -8.F );
	CHECK( seen.max.y() == 4.5F );
}

TEST_CASE( "Zoom and center choose the part of the stage that is seen", "[camera]" )
{
	Stage stage;
	stage.camera.center = { 2.F, 1.F };
	stage.camera.zoom = 2.F;

	CHECK( stage.camera.view_width() == 8.F );
	CHECK( stage.camera.view_height() == 4.5F );

	// The picture is drawn upside down (y flips), so the view's top is at minus the center's y
	const std::string svg = svg_of( stage );
	CHECK( contains( svg, "viewBox=\"-2 -3.25 8 4.5\"" ) );
	// and the background covers all of it, not where the picture used to be
	CHECK( contains( svg, "<rect x=\"-2\" y=\"-3.25\" width=\"8\" height=\"4.5\"" ) );

	const AABB seen = stage.camera.visible();
	CHECK( seen.min.x() == -2.F );
	CHECK( seen.max.x() == 6.F );
	CHECK( seen.min.y() == -1.25F );
	CHECK( seen.max.y() == 3.25F );
}

TEST_CASE( "view_between starts and ends at the views, and zooms evenly", "[camera]" )
{
	const CameraView a{ { 0.F, 0.F }, 1.F };
	const CameraView b{ { 4.F, -2.F }, 4.F };

	CHECK( view_between( a, b, 0.F ).zoom == Approx( 1.F ) );
	CHECK( view_between( a, b, 1.F ).zoom == Approx( 4.F ) );
	CHECK( view_between( a, b, 1.F ).center.x() == Approx( 4.F ) );
	CHECK( view_between( a, b, 1.F ).center.y() == Approx( -2.F ) );

	// Halfway is twice as close, not 2.5 times: zooming goes by ratios
	CHECK( view_between( a, b, 0.5F ).zoom == Approx( 2.F ) );

	// Only panning: straight there
	const CameraView c{ { 6.F, 2.F }, 1.F };
	CHECK( view_between( a, c, 0.25F ).center.x() == Approx( 1.5F ) );
	CHECK( view_between( a, c, 0.25F ).center.y() == Approx( 0.5F ) );
	CHECK( view_between( a, c, 0.25F ).zoom == Approx( 1.F ) );
}

TEST_CASE( "Zooming toward something holds it still on the screen", "[camera]" )
{
	const CameraView a{ { 0.F, 0.F }, 1.F };
	const CameraView b{ { 4.F, -2.F }, 4.F };

	// The point the two views agree on: where it lands on the picture is the same in both, and all along
	const Point fixed = ( b.center * b.zoom - a.center * a.zoom ) / ( b.zoom - a.zoom );
	CHECK( on_screen( a, fixed ).x() == Approx( on_screen( b, fixed ).x() ) );

	const Point start = on_screen( a, fixed );
	for ( float t: { 0.1F, 0.3F, 0.5F, 0.8F, 1.F } )
	{
		const Point now = on_screen( view_between( a, b, t ), fixed );
		INFO( "t = " << t );
		CHECK( now.x() == Approx( start.x() ).margin( 1e-4 ) );
		CHECK( now.y() == Approx( start.y() ).margin( 1e-4 ) );
	}
}

TEST_CASE( "view_framing fits a box with a margin, whichever way it is tight", "[camera]" )
{
	Camera camera; // 16 : 9

	// Wide: 4 across is the tight way, and 10 % of it either side
	AABB wide;
	wide.add( { 1.F, 1.F } );
	wide.add( { 5.F, 3.F } );
	CameraView view = view_framing( camera, wide );
	CHECK( view.center.x() == Approx( 3.F ) );
	CHECK( view.center.y() == Approx( 2.F ) );
	CHECK( view.zoom == Approx( 16.F / ( 4.F * 1.2F ) ) );

	// Tall: 6 up is the tight way (the picture is 9 up for 16 across)
	AABB tall;
	tall.add( { 0.F, 0.F } );
	tall.add( { 1.F, 6.F } );
	view = view_framing( camera, tall );
	CHECK( view.zoom == Approx( 9.F / ( 6.F * 1.2F ) ) );

	// Not closer than asked, and nothing to fit leaves the camera as it is
	AABB tiny;
	tiny.add( { 0.F, 0.F } );
	tiny.add( { 1e-3F, 1e-3F } );
	CHECK( view_framing( camera, tiny, 0.1F, 20.F ).zoom == 20.F );
	CHECK( view_framing( camera, AABB{} ).zoom == 1.F );
}

TEST_CASE( "camera_to moves the camera there over the time asked, and ends exactly there", "[camera]" )
{
	Stage stage;
	stage.camera.frame_rate = 24;

	Coroutine move = camera_to( stage, { { 5.F, 0.F }, 2.F }, 1s );
	move.resume();
	CHECK( stage.camera.zoom == Approx( 1.F ) ); // the first frame is where it starts

	size_t frames = 1;
	float last_zoom = stage.camera.zoom;
	while ( !move.done() )
	{
		move.resume();
		++frames;
		CHECK( stage.camera.zoom >= last_zoom - 1e-6F ); // never backwards
		last_zoom = stage.camera.zoom;
	}
	CHECK( frames == 25 ); // 24 frames of it, and the one that finds it done
	CHECK( stage.camera.zoom == 2.F );
	CHECK( stage.camera.center.x() == 5.F );
}

TEST_CASE( "camera_frame fits a visual on the stage", "[camera]" )
{
	Stage stage;
	auto box = std::make_unique<Rectangle>();
	box->half_width = 2.F;
	box->half_height = 1.F;
	box->position = { 3.F, -1.F };
	auto &shape = stage.visuals.add( std::move( box ) );

	run( camera_frame( stage, shape, 0.5s ) );
	CHECK( stage.camera.center.x() == Approx( 3.F ) );
	CHECK( stage.camera.center.y() == Approx( -1.F ) );
	CHECK( stage.camera.zoom == Approx( 16.F / ( 4.F * 1.2F ) ) );

	// What is seen now includes all of the visual, with room round it
	const AABB seen = stage.camera.visible();
	CHECK( seen.min.x() < 1.F );
	CHECK( seen.max.x() > 5.F );
}
