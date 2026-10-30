#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <core/Ease.h>
#include <core/EaseBack.h>
#include <core/EaseCubic.h>
#include <core/EaseElastic.h>
#include <core/EaseLinear.h>
#include <core/Stage.h>
#include <core/Tween.h>

#include <chrono>
#include <string>
#include <vector>

using Catch::Matchers::WithinAbs;
using namespace std::chrono_literals;

namespace
{
struct NamedEase
{
	std::string name;
	EaseFunction ease;
};

std::vector<NamedEase> all_eases()
{
	return {
		{ "linear", ease_linear },
		{ "cubic in", ease_cubic_in },
		{ "cubic out", ease_cubic_out },
		{ "cubic in out", ease_cubic_in_out },
		{ "back in", ease_back_in },
		{ "back out", ease_back_out },
		{ "back in out", ease_back_in_out },
		{ "elastic in", ease_elastic_in },
		{ "elastic out", ease_elastic_out },
		{ "elastic in out", ease_elastic_in_out },
	};
}

// Run a scene to its end, one frame at a time
void run( Coroutine scene )
{
	scene.resume();
	while ( !scene.done() )
	{
		scene.resume();
	}
}
} // namespace

TEST_CASE( "every ease starts at 0 and ends at 1", "[ease]" )
{
	for ( const auto &[name, ease]: all_eases() )
	{
		INFO( name );
		CHECK_THAT( ease( 0.F ), WithinAbs( 0.0, 1e-5 ) );
		CHECK_THAT( ease( 1.F ), WithinAbs( 1.0, 1e-5 ) );
	}
}

TEST_CASE( "the cubic and linear eases at known points", "[ease]" )
{
	CHECK_THAT( ease_linear( 0.3F ), WithinAbs( 0.3, 1e-6 ) );

	CHECK_THAT( ease_cubic_in( 0.5F ), WithinAbs( 0.125, 1e-6 ) );
	CHECK_THAT( ease_cubic_out( 0.5F ), WithinAbs( 0.875, 1e-6 ) );

	CHECK_THAT( ease_cubic_in_out( 0.25F ), WithinAbs( 0.0625, 1e-6 ) );
	CHECK_THAT( ease_cubic_in_out( 0.5F ), WithinAbs( 0.5, 1e-6 ) );
	CHECK_THAT( ease_cubic_in_out( 0.75F ), WithinAbs( 0.9375, 1e-6 ) );
}

TEST_CASE( "an out ease is an in ease played backwards", "[ease]" )
{
	for ( float t = 0.F; t <= 1.F; t += 0.05F )
	{
		CHECK_THAT( ease_cubic_out( t ), WithinAbs( 1.0 - ease_cubic_in( 1.F - t ), 1e-5 ) );
		CHECK_THAT( ease_back_out( t ), WithinAbs( 1.0 - ease_back_in( 1.F - t ), 1e-5 ) );
	}
}

TEST_CASE( "an in-out ease is the same going in as going out", "[ease]" )
{
	const std::vector<EaseFunction> in_outs = { ease_cubic_in_out, ease_back_in_out, ease_elastic_in_out };
	for ( const auto &ease: in_outs )
	{
		for ( float t = 0.F; t <= 1.F; t += 0.05F )
		{
			CHECK_THAT( ease( t ) + ease( 1.F - t ), WithinAbs( 1.0, 1e-4 ) );
		}
	}
}

TEST_CASE( "the cubic eases only ever go forward", "[ease]" )
{
	for ( const auto &ease: { EaseFunction( ease_cubic_in ), EaseFunction( ease_cubic_out ), EaseFunction( ease_cubic_in_out ) } )
	{
		float last = 0.F;
		for ( float t = 0.F; t <= 1.F; t += 0.01F )
		{
			CHECK( ease( t ) >= last );
			last = ease( t );
		}
	}
}

TEST_CASE( "the back and elastic eases go outside 0 to 1, and the others do not", "[ease]" )
{
	CHECK( ease_back_in( 0.2F ) < 0.F );   // pulls back first
	CHECK( ease_back_out( 0.8F ) > 1.F );  // overshoots
	CHECK( ease_elastic_out( 0.015F ) > 1.2F ); // its swings are all in the first few percent of the time
	CHECK( ease_elastic_in( 0.985F ) < -0.2F );

	for ( float t = 0.F; t <= 1.F; t += 0.01F )
	{
		CHECK( ease_cubic_in_out( t ) >= 0.F );
		CHECK( ease_cubic_in_out( t ) <= 1.F );
	}
}

TEST_CASE( "the back ease's overshoot can be changed, and none is a plain cubic", "[ease]" )
{
	CHECK_THAT( EaseBackIn{ 0.F }( 0.5F ), WithinAbs( 0.125, 1e-6 ) );
	CHECK( EaseBackIn{ 3.F }( 0.2F ) < ease_back_in( 0.2F ) ); // pulls back further
}

TEST_CASE( "tween gives t from 0 to 1 over the frames a time takes", "[ease][tween]" )
{
	Stage stage;
	std::vector<float> values;
	auto record = [&]( float t ) { values.push_back( t ); };

	stage.camera.frame_rate = 24;
	run( tween( stage, 1s, ease_linear, record ) );
	REQUIRE( values.size() == 24 );
	CHECK( values.front() == 0.F );
	CHECK( values.back() == 1.F );
	CHECK_THAT( values[12], WithinAbs( 12.0 / 23.0, 1e-6 ) );

	values.clear();
	stage.camera.frame_rate = 60;
	run( tween( stage, 1s, ease_linear, record ) );
	CHECK( values.size() == 60 );
	CHECK( values.back() == 1.F );
}

TEST_CASE( "tween applies the ease, and is never under two frames", "[ease][tween]" )
{
	Stage stage;
	stage.camera.frame_rate = 24;

	std::vector<float> values;
	run( tween( stage, 0.125s, ease_cubic_in, [&]( float t ) { values.push_back( t ); } ) ); // 3 frames: 0, 1/2, 1
	REQUIRE( values.size() == 3 );
	CHECK_THAT( values[1], WithinAbs( 0.125, 1e-6 ) );

	values.clear();
	run( tween( stage, 0s, ease_linear, [&]( float t ) { values.push_back( t ); } ) );
	REQUIRE( values.size() == 2 );
	CHECK( values[0] == 0.F );
	CHECK( values[1] == 1.F );
}
