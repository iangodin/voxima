#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "RenderTestUtil.h"

#include <core/Stage.h>
#include <visuals/Arrow.h>
#include <visuals/Geometry.h>
#include <visuals/Morph.h>
#include <visuals/Trim.h>

#include <chrono>
#include <cmath>
#include <variant>

using Catch::Approx;
using namespace render_test;
using namespace std::chrono_literals;

namespace
{
float distance( Point a, Point b )
{
	return std::sqrt( ( b - a ).lengthSquared() );
}

ConnectorOptions with_head( float size )
{
	ConnectorOptions options;
	options.arrow_size = size;
	return options;
}
}

TEST_CASE( "An arrow goes from one point to the other, its head at the end", "[arrow]" )
{
	const auto arrow = make_arrow( { -2.F, 0.F }, { 2.F, 0.F }, with_head( 0.25F ) );
	CHECK( arrow->end_tip().end.x() == Approx( 2.F ) );
	CHECK( arrow->end_tip().size == Approx( 0.25F ) );
	CHECK( arrow->start_tip().size == 0.F );

	// The line stops where the head starts, so it does not show past the point
	const Contour line = contour_of( arrow->shaft() );
	CHECK( line.start.x() == Approx( -2.F ) );
	CHECK( line.segs.back().to.x() == Approx( 1.5F ) );
	CHECK( arrow->end_tip().start.x() == Approx( 1.5F ) );

	const auto picture = render( *arrow );
	CHECK( pixel_at( picture, 0.F, 0.F ).r > 200 );
	CHECK( pixel_at( picture, 1.6F, 0.15F ).r > 200 ); // the head is wider than the line
	CHECK( pixel_at( picture, 0.F, 0.15F ).r == 0 );
}

TEST_CASE( "An arrow takes a connector's options", "[arrow]" )
{
	ConnectorOptions options;
	options.arrow_size = 0.25F;
	options.thickness = 0.2F;
	options.color = { 1.F, 0.F, 0.F, 1.F };
	options.dash = StrokeDash::dashed( 0.3F, 0.1F );

	SECTION( "colors and stroke" )
	{
		const auto arrow = make_arrow( { 0.F, 0.F }, { 2.F, 0.F }, options );
		CHECK( arrow->shaft().stroke_thickness == 0.2F );
		CHECK( arrow->shaft().stroke_color == options.color );
		CHECK( arrow->shaft().color.a == 0.F );
		CHECK( arrow->end_tip().color == options.color );
		CHECK( arrow->shaft().stroke_dash == options.dash ); // and the heads are whole
		CHECK( arrow->end_tip().stroke_dash.solid() );
	}
	SECTION( "a head at the start as well" )
	{
		options.arrow_at_start = true;
		const auto arrow = make_arrow( { 0.F, 0.F }, { 4.F, 0.F }, options );
		CHECK( arrow->start_tip().size == Approx( 0.25F ) );
		CHECK( arrow->start_tip().end.x() == Approx( 0.F ).margin( 1e-5 ) );
		CHECK( arrow->start_tip().start.x() == Approx( 0.5F ) );
		const Contour line = contour_of( arrow->shaft() );
		CHECK( line.start.x() == Approx( 0.5F ) );
		CHECK( line.segs.back().to.x() == Approx( 3.5F ) );
	}
	SECTION( "no head at all" )
	{
		options.arrow = false;
		const auto arrow = make_arrow( { 0.F, 0.F }, { 2.F, 0.F }, options );
		CHECK( arrow->end_tip().size == 0.F );
		CHECK( contour_of( arrow->shaft() ).segs.back().to.x() == Approx( 2.F ) );
	}
	SECTION( "a head at the start only" )
	{
		options.arrow = false;
		options.arrow_at_start = true;
		const auto arrow = make_arrow( { 0.F, 0.F }, { 2.F, 0.F }, options );
		CHECK( arrow->end_tip().size == 0.F );
		CHECK( arrow->start_tip().size == Approx( 0.25F ) );
		CHECK( contour_of( arrow->shaft() ).segs.back().to.x() == Approx( 2.F ) );
		CHECK( contour_of( arrow->shaft() ).start.x() == Approx( 0.5F ) );
	}
}

TEST_CASE( "An arrow too short for its head has none", "[arrow]" )
{
	const auto arrow = make_arrow( { 0.F, 0.F }, { 0.2F, 0.F }, with_head( 0.25F ) );
	CHECK( arrow->end_tip().size == 0.F );
	CHECK( contour_of( arrow->shaft() ).segs.back().to.x() == Approx( 0.2F ) );

	const auto none = make_arrow( { 1.F, 1.F }, { 1.F, 1.F } );
	CHECK( none->end_tip().size == 0.F );
	CHECK_FALSE( render( *none ).isNull() );
}

TEST_CASE( "A curved arrow's head points along the curve", "[arrow]" )
{
	// A quarter circle of radius 2, up from the right: at its end it is heading left
	const auto arc = make_arc( { 0.F, 0.F }, 2.F, 0.F, 90.F );
	const auto arrow = make_arrow( *arc, with_head( 0.1F ) );
	const ArrowTip &tip = arrow->end_tip();
	CHECK( tip.end.x() == Approx( 0.F ).margin( 1e-4 ) );
	CHECK( tip.end.y() == Approx( 2.F ) );
	CHECK( tip.start.x() > tip.end.x() );
	CHECK( std::abs( tip.start.y() - tip.end.y() ) < std::abs( tip.start.x() - tip.end.x() ) * 0.2F );
	CHECK( distance( tip.start, tip.end ) == Approx( 0.2F ).margin( 0.01F ) );

	// The line ends at the head's base, on the circle
	const Contour line = contour_of( arrow->shaft() );
	CHECK( distance( line.segs.back().to, tip.start ) < 0.001F );
	CHECK( distance( line.segs.back().to, { 0.F, 0.F } ) == Approx( 2.F ).margin( 0.01F ) );
}

TEST_CASE( "An arrow along a path is put where the path is", "[arrow]" )
{
	Path path;
	path.commands = { MoveTo{ { 0.F, 0.F } }, LineTo{ { 2.F, 0.F } } };
	path.position = { 1.F, 2.F };
	path.rotation = 90.F;
	path.scale_x = 2.F;
	const auto arrow = make_arrow( path );
	CHECK( arrow->position == path.position );
	CHECK( arrow->rotation == 90.F );
	CHECK( arrow->scale_x == 2.F );
}

TEST_CASE( "An arrow along a closed path still has its two ends", "[arrow]" )
{
	Path path;
	path.commands = { MoveTo{ { 0.F, 0.F } }, LineTo{ { 2.F, 0.F } }, LineTo{ { 2.F, 2.F } }, Close{} };
	const auto arrow = make_arrow( path, with_head( 0.1F ) );
	CHECK( arrow->end_tip().size == Approx( 0.1F ) );
	CHECK_FALSE( contour_of( arrow->shaft() ).closed );
}

TEST_CASE( "An arrow is drawn on: the line, then the head", "[arrow]" )
{
	const auto arrow = make_arrow( { 0.F, 0.F }, { 4.F, 0.F }, with_head( 0.25F ) );

	arrow->set_drawn( 0.F );
	CHECK( arrow->shaft().commands.size() == 1 ); // nowhere yet
	CHECK( arrow->end_tip().size == 0.F );

	arrow->set_drawn( 0.4F ); // halfway along the line
	CHECK( contour_of( arrow->shaft() ).segs.back().to.x() == Approx( 1.75F ).margin( 0.01F ) );
	CHECK( arrow->end_tip().size == 0.F );

	arrow->set_drawn( 0.8F ); // the whole line, and the head is about to start
	CHECK( contour_of( arrow->shaft() ).segs.back().to.x() == Approx( 3.5F ) );
	CHECK( arrow->end_tip().size == 0.F );

	arrow->set_drawn( 0.9F ); // the head halfway, growing from the end of the line
	CHECK( arrow->end_tip().size == Approx( 0.125F ) );
	CHECK( arrow->end_tip().start.x() == Approx( 3.5F ) );
	CHECK( arrow->end_tip().end.x() == Approx( 3.75F ) );

	arrow->set_drawn( 1.F );
	CHECK( arrow->end_tip().size == Approx( 0.25F ) );
	CHECK( arrow->end_tip().end.x() == Approx( 4.F ) );
	CHECK( arrow->drawn() == 1.F );

	arrow->set_drawn( 5.F );
	CHECK( arrow->drawn() == 1.F );
}

TEST_CASE( "With a head at the start, that is drawn first", "[arrow]" )
{
	ConnectorOptions options = with_head( 0.25F );
	options.arrow_at_start = true;
	const auto arrow = make_arrow( { 0.F, 0.F }, { 4.F, 0.F }, options );
	arrow->set_drawn( 0.1F );
	CHECK( arrow->start_tip().size == Approx( 0.125F ) );
	CHECK( arrow->shaft().commands.size() == 1 );
	CHECK( arrow->end_tip().size == 0.F );
	arrow->set_drawn( 0.2F );
	CHECK( arrow->start_tip().size == Approx( 0.25F ) );
}

TEST_CASE( "draw plays an arrow from nothing to all of it", "[arrow]" )
{
	Stage stage;
	const auto arrow = make_arrow( { 0.F, 0.F }, { 4.F, 0.F } );
	Scene scene = draw( stage, *arrow, 0.5s );
	scene.resume();
	CHECK( arrow->drawn() == 0.F );
	while ( !scene.done() )
	{
		scene.resume();
	}
	CHECK( arrow->drawn() == 1.F );
}

TEST_CASE( "Scaling an arrow scales it, and it can still be drawn on", "[arrow]" )
{
	const auto arrow = make_arrow( { 0.F, 0.F }, { 4.F, 0.F }, with_head( 0.25F ) );
	*arrow *= 0.5F;
	CHECK( arrow->end_tip().end.x() == Approx( 2.F ) );
	CHECK( arrow->end_tip().size == Approx( 0.125F ) );
	arrow->set_drawn( 0.8F );
	CHECK( contour_of( arrow->shaft() ).segs.back().to.x() == Approx( 1.75F ) );
}

TEST_CASE( "An arrow morphs", "[arrow][morph]" )
{
	const auto a = make_arrow( { -2.F, 0.F }, { 2.F, 0.F } );
	const auto b = make_arrow( *make_arc( { 0.F, 0.F }, 2.F, 0.F, 120.F ) );
	Morph morph( *a, *b );
	morph.set( 0.F );
	CHECK( pixels_apart( render( morph.visual() ), render( *a ) ) < 40 );
	morph.set( 1.F );
	CHECK( pixels_apart( render( morph.visual() ), render( *b ) ) < 40 );
}
