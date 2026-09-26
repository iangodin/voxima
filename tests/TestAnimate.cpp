#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Animate.h>
#include <core/Stage.h>
#include <visuals/Rectangle.h>

#include <chrono>

using Catch::Approx;
using namespace std::chrono_literals;

namespace
{
void run( Scene scene )
{
	scene.resume();
	while ( !scene.done() )
	{
		scene.resume();
	}
}

Rectangle box()
{
	Rectangle r;
	r.half_width = 1.F;
	r.half_height = 0.5F;
	r.position = { 3.F, 1.F };
	return r;
}
}

TEST_CASE( "fade_in and fade_out end at what they are meant to", "[animate]" )
{
	Stage stage;
	Rectangle r = box();
	r.color.a = 0.5F;
	run( fade_in( stage, r, 0.2s ) );
	CHECK( r.color.a == Approx( 0.5F ) );
	run( fade_out( stage, r, 0.2s ) );
	CHECK( r.color.a == Approx( 0.F ) );
}

TEST_CASE( "fade_in starts from nothing", "[animate]" )
{
	Stage stage;
	Rectangle r = box();
	Scene scene = fade_in( stage, r, 0.2s );
	scene.resume();
	CHECK( r.color.a == Approx( 0.F ) );
	run( std::move( scene ) );
}

TEST_CASE( "slide_in ends where it is, slide_out where it is sent", "[animate]" )
{
	Stage stage;
	Rectangle r = box();
	run( slide_in( stage, r, { 4.F, 0.F }, 0.2s ) );
	CHECK( r.position.x() == Approx( 3.F ) );
	run( slide_out( stage, r, { 0.F, 2.F }, 0.2s ) );
	CHECK( r.position.y() == Approx( 3.F ) );
	CHECK( r.color.a == Approx( 0.F ) );
}

TEST_CASE( "indicate leaves it as it was, pulsing about its center", "[animate]" )
{
	Stage stage;
	Rectangle r = box();
	Scene scene = indicate( stage, r, 1.5F, 0.4s );
	float biggest = 0.F;
	scene.resume();
	while ( !scene.done() )
	{
		biggest = std::max( biggest, r.scale_x );
		// the center of the box stays put (the box is centered on the origin here)
		CHECK( r.position.x() == Approx( 3.F ) );
		scene.resume();
	}
	CHECK( biggest > 1.4F );
	CHECK( r.scale_x == Approx( 1.F ) );
	CHECK( r.position.x() == Approx( 3.F ) );
}

TEST_CASE( "grow_in and shrink_out", "[animate]" )
{
	Stage stage;
	Rectangle r = box();
	run( grow_in( stage, r, 0.2s ) );
	CHECK( r.scale_x == Approx( 1.F ) );
	CHECK( r.color.a == Approx( 1.F ) );
	run( shrink_out( stage, r, 0.2s ) );
	CHECK( r.scale_x < 0.01F );
	CHECK( r.color.a == Approx( 0.F ) );
}

TEST_CASE( "circumscribe and flash leave the stage as it was", "[animate]" )
{
	Stage stage;
	auto &r = stage.visuals.add( std::make_unique<Rectangle>( box() ) );
	const size_t before = stage.visuals.size();

	Scene scene = circumscribe( stage, r, {}, 0.2s );
	scene.resume();
	CHECK( stage.visuals.size() == before + 1 ); // the line being drawn
	run( std::move( scene ) );
	CHECK( stage.visuals.size() == before );

	run( flash( stage, r, {}, 0.2s ) );
	CHECK( stage.visuals.size() == before );
}
