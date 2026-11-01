#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <core/Coroutine.h>
#include <core/Director.h>
#include <core/Scene.h>
#include <core/Wait.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace
{
void run( Scene scene )
{
	while ( !scene.done() )
	{
		scene.resume();
	}
}

Scene throws_now()
{
	throw std::runtime_error( "now" );
	co_return;
}

Scene throws_later()
{
	co_await next_frame();
	co_await next_frame();
	throw std::runtime_error( "later" );
}

Scene waits_for( Scene scene )
{
	co_await next_frame();
	co_await scene;
}

Scene waits_deep()
{
	co_await waits_for( waits_for( throws_later() ) );
}
}

TEST_CASE( "What a scene throws reaches whoever is running it", "[scenes][errors]" )
{
	CHECK_THROWS_WITH( run( throws_now() ), "now" );
	CHECK_THROWS_WITH( run( throws_later() ), "later" );
}

TEST_CASE( "What a scene throws goes through the scenes waiting for it", "[scenes][errors]" )
{
	CHECK_THROWS_WITH( run( waits_for( throws_now() ) ), "now" );
	CHECK_THROWS_WITH( run( waits_deep() ), "later" );
}

TEST_CASE( "What a scene throws goes through a choreograph", "[scenes][errors]" )
{
	const auto scene = []() -> Scene { co_await choreograph( wait_frames( 10 ), throws_later() ); };
	CHECK_THROWS_WITH( run( scene() ), "later" );

	const auto all = []() -> Scene
	{
		std::vector<Coroutine> scenes;
		scenes.push_back( wait_frames( 10 ) );
		scenes.push_back( throws_later() );
		co_await choreograph_all( std::move( scenes ) );
	};
	CHECK_THROWS_WITH( run( all() ), "later" );
}

TEST_CASE( "A scene can catch what one it waits for throws, and go on", "[scenes][errors]" )
{
	std::string caught;
	bool went_on = false;
	const auto scene = [&]() -> Scene
	{
		try
		{
			co_await throws_later();
		}
		catch ( const std::runtime_error &error )
		{
			caught = error.what();
		}
		co_await next_frame();
		went_on = true;
	};
	run( scene() );
	CHECK( caught == "later" );
	CHECK( went_on );
}

TEST_CASE( "A scene that goes on normally is not affected", "[scenes][errors]" )
{
	int count = 0;
	const auto scene = [&]() -> Scene
	{
		for ( int i = 0; i < 5; ++i )
		{
			co_await next_frame();
			++count;
		}
	};
	run( waits_for( scene() ) );
	CHECK( count == 5 );
}
