#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "RenderTestUtil.h"

#include <core/Animate.h>
#include <core/EaseThereAndBack.h>
#include <core/Stage.h>
#include <core/Tween.h>
#include <visuals/Circle.h>
#include <visuals/Geometry.h>
#include <visuals/Panel.h>
#include <visuals/Rectangle.h>

#include <chrono>
#include <cmath>
#include <variant>

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

Point end_of( const Path &path )
{
	return std::visit(
		[]( const auto &command ) -> Point
		{
			if constexpr ( requires { command.to; } )
			{
				return command.to;
			}
			else
			{
				return {};
			}
		},
		path.commands.back() );
}
}

TEST_CASE( "make_surround goes round a visual's box", "[helpers]" )
{
	Rectangle box( 1.F, 0.5F );
	box.position = { 2.F, 1.F };
	const auto frame = make_surround( box, { .padding = 0.25F } );
	CHECK( frame->position.x() == Approx( 2.F ) );
	CHECK( frame->position.y() == Approx( 1.F ) );
	CHECK( frame->half_width == Approx( 1.25F ) );
	CHECK( frame->half_height == Approx( 0.75F ) );
	CHECK( frame->color.a == 0.F );
	CHECK( frame->stroke_thickness > 0.F );
}

TEST_CASE( "make_surround goes round a visual as it is drawn, scaled", "[helpers]" )
{
	Rectangle box( 1.F, 1.F );
	box.position = { 1.F, 0.F };
	box.scale_x = 2.F;
	const auto frame = make_surround( box, { .padding = 0.F } );
	CHECK( frame->position.x() == Approx( 1.F ) );
	CHECK( frame->half_width == Approx( 2.F ) );
	CHECK( frame->half_height == Approx( 1.F ) );
}

TEST_CASE( "make_surround keeps its options, and its corners in reason", "[helpers]" )
{
	Rectangle box( 1.F, 1.F );
	const Color yellow{ 1.F, 1.F, 0.F, 1.F };
	SurroundOptions options;
	options.padding = 0.F;
	options.radius = 0.3F;
	options.outline = yellow;
	options.dash = StrokeDash::dashed( 0.1F, 0.1F );
	auto frame = make_surround( box, options );
	CHECK( frame->top_left_radius == Approx( 0.3F ) );
	CHECK( frame->stroke_color == yellow );
	CHECK( frame->stroke_dash == options.dash );

	options.radius = 50.F;
	frame = make_surround( box, options );
	CHECK( frame->top_left_radius == Approx( 1.F ) );

	const Group empty;
	frame = make_surround( empty );
	CHECK( frame->half_width == 0.F );
}

TEST_CASE( "make_arc is on the circle, between its angles", "[helpers]" )
{
	const Point center{ 1.F, 2.F };
	const auto arc = make_arc( center, 2.F, 0.F, 90.F );
	REQUIRE( arc->commands.size() == 2 );
	const auto *start = std::get_if<MoveTo>( &arc->commands[0] );
	REQUIRE( start );
	CHECK( start->to.x() == Approx( 3.F ) );
	CHECK( start->to.y() == Approx( 2.F ) );
	CHECK( end_of( *arc ).x() == Approx( 1.F ) );
	CHECK( end_of( *arc ).y() == Approx( 4.F ) );
	CHECK( arc->color.a == 0.F ); // only a stroke

	// and stays on it in between
	const auto *curve = std::get_if<CubicTo>( &arc->commands[1] );
	REQUIRE( curve );
	const auto at = [&]( float t )
	{
		const float u = 1.F - t;
		return start->to * ( u * u * u ) + curve->ctrl1 * ( 3.F * u * u * t ) + curve->ctrl2 * ( 3.F * u * t * t ) + curve->to * ( t * t * t );
	};
	for ( const float t: { 0.25F, 0.5F, 0.75F } )
	{
		CHECK( std::sqrt( ( at( t ) - center ).lengthSquared() ) == Approx( 2.F ).margin( 0.002F ) );
	}
}

TEST_CASE( "make_arc goes the way its angles say, and all the way round", "[helpers]" )
{
	const auto backward = make_arc( { 0.F, 0.F }, 1.F, 90.F, 0.F );
	CHECK( end_of( *backward ).x() == Approx( 1.F ) );
	CHECK( end_of( *backward ).y() == Approx( 0.F ).margin( 1e-5 ) );

	const auto round = make_arc( { 0.F, 0.F }, 1.F, 0.F, 360.F );
	CHECK( round->commands.size() == 5 );
	CHECK( end_of( *round ).x() == Approx( 1.F ) );

	const auto none = make_arc( { 0.F, 0.F }, 1.F, 45.F, 45.F );
	CHECK( none->commands.size() == 1 );
}

TEST_CASE( "An arc is drawn where it should be", "[helpers]" )
{
	const auto arc = make_arc( { 0.F, 0.F }, 2.F, 0.F, 90.F, { .thickness = 0.2F } );
	const auto picture = render_test::render( *arc );
	CHECK( render_test::pixel_at( picture, 2.F * std::cos( 0.785F ), 2.F * std::sin( 0.785F ) ).r == 255 ); // up and to the right
	CHECK( render_test::pixel_at( picture, -2.F * std::cos( 0.785F ), 2.F * std::sin( 0.785F ) ).r == 0 );
	CHECK( render_test::pixel_at( picture, 0.F, 0.F ).r == 0 );
}

TEST_CASE( "grow_from_edge keeps the edge where it is", "[helpers]" )
{
	Stage stage;
	Rectangle bar( 0.5F, 2.F );
	bar.position = { 1.F, 3.F };

	SECTION( "from the bottom, along y only" )
	{
		Scene scene = grow_from_edge( stage, bar, Anchor::BOTTOM, 1s );
		scene.resume();
		CHECK( bar.scale_y < 0.1F );
		CHECK( bar.scale_x == 1.F );
		const auto bottom = [&]() { return bar.position.y() - 2.F * bar.scale_y; };
		CHECK( bottom() == Approx( 1.F ) );
		CHECK( bar.position.x() == 1.F );
		for ( int i = 0; i < 5; ++i )
		{
			scene.resume();
		}
		CHECK( bar.scale_y > 0.F );
		CHECK( bottom() == Approx( 1.F ) );
		while ( !scene.done() )
		{
			scene.resume();
		}
	}
	SECTION( "from the left, along x only" )
	{
		Scene scene = grow_from_edge( stage, bar, Anchor::LEFT, 1s );
		scene.resume();
		CHECK( bar.scale_x < 0.1F );
		CHECK( bar.scale_y == 1.F );
		CHECK( bar.position.x() - 0.5F * bar.scale_x == Approx( 0.5F ) );
		while ( !scene.done() )
		{
			scene.resume();
		}
	}
	SECTION( "from a corner, both ways" )
	{
		Scene scene = grow_from_edge( stage, bar, Anchor::TOP_RIGHT, 1s );
		scene.resume();
		CHECK( bar.scale_x < 0.1F );
		CHECK( bar.scale_y < 0.1F );
		CHECK( bar.position.x() + 0.5F * bar.scale_x == Approx( 1.5F ) );
		CHECK( bar.position.y() + 2.F * bar.scale_y == Approx( 5.F ) );
		while ( !scene.done() )
		{
			scene.resume();
		}
	}
	// In any case it ends as it was
	CHECK( bar.scale_x == 1.F );
	CHECK( bar.scale_y == 1.F );
	CHECK( bar.position.x() == 1.F );
	CHECK( bar.position.y() == 3.F );
}

TEST_CASE( "grow_from_edge takes a scale into account", "[helpers]" )
{
	Stage stage;
	Rectangle bar( 0.5F, 2.F );
	bar.scale_y = 2.F; // from y = -4 to 4
	Scene scene = grow_from_edge( stage, bar, Anchor::BOTTOM, 1s );
	scene.resume();
	CHECK( bar.position.y() - 2.F * bar.scale_y == Approx( -4.F ) );
	while ( !scene.done() )
	{
		scene.resume();
	}
	CHECK( bar.scale_y == 2.F );
	CHECK( bar.position.y() == 0.F );
}

TEST_CASE( "ease_there_and_back goes out and comes back", "[helpers]" )
{
	CHECK( ease_there_and_back( 0.F ) == Approx( 0.F ) );
	CHECK( ease_there_and_back( 0.5F ) == Approx( 1.F ) );
	CHECK( ease_there_and_back( 1.F ) == Approx( 0.F ) );
	CHECK( ease_there_and_back( 0.25F ) == Approx( ease_there_and_back( 0.75F ) ) );
	CHECK( ease_there_and_back( 0.25F ) == Approx( 0.5F ) );
}

TEST_CASE( "ease_there_and_back_with_pause stays out for the pause", "[helpers]" )
{
	const EaseFunction ease = ease_there_and_back_with_pause( 0.5F );
	CHECK( ease( 0.F ) == Approx( 0.F ) );
	CHECK( ease( 0.125F ) == Approx( 0.5F ) );
	CHECK( ease( 0.25F ) == Approx( 1.F ) );
	CHECK( ease( 0.5F ) == 1.F );
	CHECK( ease( 0.75F ) == Approx( 1.F ) );
	CHECK( ease( 1.F ) == Approx( 0.F ) );

	// No pause is the plain one, and the most is a pause that still gets back
	const EaseFunction none = ease_there_and_back_with_pause( 0.F );
	CHECK( none( 0.25F ) == Approx( ease_there_and_back( 0.25F ) ) );
	CHECK( ease_there_and_back_with_pause( 5.F )( 1.F ) == Approx( 0.F ) );
}

TEST_CASE( "A tween with an ease there and back leaves things as they were", "[helpers]" )
{
	Stage stage;
	float value = 5.F;
	run( tween( stage, 0.2s, ease_there_and_back, [&]( float t ) { value = 5.F + t; } ) );
	CHECK( value == Approx( 5.F ) );
}
