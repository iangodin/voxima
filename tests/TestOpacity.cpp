#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "RenderTestUtil.h"

#include <core/Animate.h>
#include <core/Stage.h>
#include <visuals/Circle.h>
#include <visuals/Morph.h>
#include <visuals/Outline.h>
#include <visuals/Rectangle.h>

#include <chrono>
#include <memory>
#include <string>

using Catch::Approx;
using namespace render_test;
using namespace std::chrono_literals;

namespace
{
const Color RED{ 1.F, 0.F, 0.F, 1.F };
const Color BLUE{ 0.F, 0.F, 1.F, 1.F };

bool contains( const std::string &text, const std::string &needle )
{
	return text.find( needle ) != std::string::npos;
}
}

TEST_CASE( "An opaque visual writes what it wrote before", "[opacity]" )
{
	Rectangle box( 1.F, 1.F );
	CHECK_FALSE( contains( svg_of( box ), "<g opacity" ) );
	box.opacity = 0.9995F; // nearly opaque is opaque
	CHECK_FALSE( contains( svg_of( box ), "<g opacity" ) );
}

TEST_CASE( "Opacity is applied to a shape", "[opacity]" )
{
	Rectangle box( 1.F, 1.F, RED );
	box.opacity = 0.5F;
	CHECK( contains( svg_of( box ), "<g opacity=\"0.5" ) );
	CHECK( pixel_at( render( box ), 0.F, 0.F ).r == Approx( 127 ).margin( 2 ) );
}

TEST_CASE( "A group's opacity is applied once, to it as a whole", "[opacity]" )
{
	// A blue box half over a red one, the blue one in front
	Group group;
	auto &back = group.add( std::make_unique<Rectangle>( 2.F, 1.F, RED ) );
	back.position = { -1.F, 0.F };
	auto &front = group.add( std::make_unique<Rectangle>( 2.F, 1.F, BLUE ) );
	front.position = { 1.F, 0.F };

	SECTION( "with opacity, none of the red shows through the blue" )
	{
		group.opacity = 0.5F;
		const auto picture = render( group );
		CHECK( pixel_at( picture, 0.F, 0.F ).r == 0 );
		CHECK( pixel_at( picture, 0.F, 0.F ).b == Approx( 127 ).margin( 2 ) );
		CHECK( pixel_at( picture, -2.5F, 0.F ).r == Approx( 127 ).margin( 2 ) );
	}
	SECTION( "while with the colors' alpha, which goes to each child, it does" )
	{
		group.color.a = 0.5F;
		CHECK( pixel_at( render( group ), 0.F, 0.F ).r > 30 );
	}
	SECTION( "and nested ones multiply" )
	{
		group.opacity = 0.5F;
		front.opacity = 0.5F;
		const auto picture = render( group );
		CHECK( pixel_at( picture, 2.5F, 0.F ).b == Approx( 64 ).margin( 2 ) );
	}
}

TEST_CASE( "fade_to goes to an opacity, stays, and comes back", "[opacity]" )
{
	Stage stage;
	Rectangle box( 1.F, 1.F, Color{ 1.F, 0.F, 0.F, 0.5F } );
	const auto run = []( Scene scene )
	{
		scene.resume();
		while ( !scene.done() )
		{
			scene.resume();
		}
	};

	Scene scene = fade_to( stage, box, 0.3F, 1s );
	scene.resume();
	scene.resume();
	CHECK( box.opacity < 1.F );
	CHECK( box.opacity > 0.3F );
	while ( !scene.done() )
	{
		scene.resume();
	}
	CHECK( box.opacity == 0.3F );
	CHECK( box.color.a == 0.5F ); // its colors are its own

	run( fade_to( stage, box, 1.F, 0.2s ) );
	CHECK( box.opacity == 1.F );

	run( fade_to( stage, box, 2.F, 0.1s ) ); // kept in range
	CHECK( box.opacity == 1.F );
	run( fade_to( stage, box, -1.F, 0.1s ) );
	CHECK( box.opacity == 0.F );
}

TEST_CASE( "fade_in and fade_out work with what is dimmed", "[opacity]" )
{
	Stage stage;
	Rectangle box( 1.F, 1.F, RED );
	box.opacity = 0.3F;
	Scene scene = fade_out( stage, box, 0.1s );
	while ( !scene.done() )
	{
		scene.resume();
	}
	CHECK( box.opacity == 0.3F );
	CHECK( box.color.a == 0.F );
}

TEST_CASE( "What is flattened has its opacity in its colors", "[opacity][morph]" )
{
	Group group;
	group.opacity = 0.5F;
	auto &box = group.add( std::make_unique<Rectangle>( 1.F, 1.F, RED ) );
	box.opacity = 0.5F;
	box.stroke_thickness = 0.1F;
	const auto shapes = to_outline( group );
	REQUIRE( shapes.size() == 1 );
	CHECK( shapes[0].fill.a == Approx( 0.25F ) );
	CHECK( shapes[0].stroke.a == Approx( 0.25F ) );

	// So a morph of what is dimmed starts as dim
	Rectangle dim( 1.F, 1.F, RED );
	dim.opacity = 0.5F;
	Circle other( 1.F, RED );
	Morph morph( dim, other );
	CHECK( pixels_apart( render( morph.visual() ), render( dim ), 16 ) < 40 );
}
