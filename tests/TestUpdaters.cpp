#include <catch2/catch_test_macros.hpp>

#include <core/Updaters.h>

TEST_CASE( "Updaters run in the order they were added", "[updaters]" )
{
	Updaters updaters;
	std::string order;
	auto a = updaters.add( [&] { order += 'a'; } );
	auto b = updaters.add( [&] { order += 'b'; } );
	updaters.run();
	updaters.run();
	CHECK( order == "abab" );
}

TEST_CASE( "A handle ends its updater when it goes", "[updaters]" )
{
	Updaters updaters;
	int count = 0;
	{
		auto handle = updaters.add( [&] { ++count; } );
		updaters.run();
		CHECK( updaters.size() == 1 );
	}
	updaters.run();
	CHECK( count == 1 );
	CHECK( updaters.size() == 0 );
}

TEST_CASE( "A released updater outlives its handle, until cleared", "[updaters]" )
{
	Updaters updaters;
	int count = 0;
	{
		auto handle = updaters.add( [&] { ++count; } );
		handle.release();
	}
	updaters.run();
	updaters.run();
	CHECK( count == 2 );
	updaters.clear();
	updaters.run();
	CHECK( count == 2 );
}

TEST_CASE( "An updater may end itself or add another while running", "[updaters]" )
{
	Updaters updaters;
	int once = 0;
	int added = 0;
	Updaters::Handle self;
	Updaters::Handle other;
	self = updaters.add( [&] { ++once; self.stop(); other = updaters.add( [&] { ++added; } ); } );
	updaters.run();
	CHECK( once == 1 );
	CHECK( added == 0 ); // (from the next frame)
	updaters.run();
	CHECK( once == 1 );
	CHECK( added == 1 );
}
