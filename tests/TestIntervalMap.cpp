
#include <catch2/catch_test_macros.hpp>
#include <utils/IntervalMap.h>

SCENARIO( "IntervalMap basic operations" )
{
	GIVEN( "An empty IntervalMap" )
	{
		IntervalMap<int, std::string> map;

		THEN( "It is empty" )
		{
			CHECK( map.empty() );
			CHECK( map.size() == 0 );
			CHECK( !map.contains( 0 ) );
			CHECK( !map.find( 0 ).has_value() );
			CHECK( !map.span().has_value() );
		}

		WHEN( "Inserting a single interval" )
		{
			map.insert( 10, 20, "ten-twenty" );

			THEN( "It contains the interval" )
			{
				CHECK( !map.empty() );
				CHECK( map.size() == 1 );
				CHECK( map.contains( 10 ) );
				CHECK( map.contains( 15 ) );
				CHECK( map.contains( 19 ) );
				CHECK( !map.contains( 9 ) );
				CHECK( !map.contains( 20 ) );

				auto val = map.find( 15 );
				REQUIRE( val.has_value() );
				CHECK( *val == "ten-twenty" );

				auto span = map.span();
				REQUIRE( span.has_value() );
				CHECK( span->first == 10 );
				CHECK( span->second == 20 );
			}
		}

		WHEN( "Inserting multiple non-overlapping intervals" )
		{
			map.insert( 10, 20, "A" );
			map.insert( 30, 40, "B" );

			THEN( "Both are present" )
			{
				CHECK( map.size() == 2 );
				CHECK( map.find( 15 ) == "A" );
				CHECK( map.find( 35 ) == "B" );
				CHECK( !map.find( 25 ).has_value() );
			}
		}
	}
}

SCENARIO( "IntervalMap overlap behavior" )
{
	GIVEN( "An IntervalMap with some data" )
	{
		IntervalMap<int, std::string> map;
		map.insert( 10, 20, "A" );
		map.insert( 30, 40, "B" );

		WHEN( "Inserting an interval that overlaps exactly with an existing one" )
		{
			map.insert( 10, 20, "A_new" );
			THEN( "It is replaced" )
			{
				CHECK( map.size() == 2 );
				CHECK( map.find( 15 ) == "A_new" );
				CHECK( map.find( 35 ) == "B" );
			}
		}

		WHEN( "Inserting an interval that overlaps partially (start)" )
		{
			// [10, 20) is "A". Insert [5, 15) "C".
			// Tails are preserved: [5, 15) is "C", [15, 20) is "A".
			map.insert( 5, 15, "C" );
			THEN( "The overlapping part is replaced, and the tail is preserved" )
			{
				CHECK( map.find( 5 ) == "C" );
				CHECK( map.find( 14 ) == "C" );
				CHECK( map.find( 15 ) == "A" );
				CHECK( map.find( 19 ) == "A" );
				CHECK( map.size() == 3 ); // [5, 15) "C", [15, 20) "A", [30, 40) "B"
				
				auto span = map.span();
				CHECK( span->first == 5 );
				CHECK( span->second == 40 );
			}
		}

		WHEN( "Inserting an interval that overlaps partially (end)" )
		{
			// [30, 40) is "B". Insert [35, 45) "D".
			// Tails are preserved: [30, 35) is "B", [35, 45) is "D".
			map.insert( 35, 45, "D" );
			THEN( "The overlapping part is replaced, and the head is preserved" )
			{
				CHECK( map.find( 30 ) == "B" );
				CHECK( map.find( 34 ) == "B" );
				CHECK( map.find( 35 ) == "D" );
				CHECK( map.find( 44 ) == "D" );
				CHECK( map.size() == 3 ); // [10, 20) "A", [30, 35) "B", [35, 45) "D"
			}
		}

		WHEN( "Inserting an interval that is fully inside an existing one" )
		{
			// [10, 20) is "A". Insert [12, 18) "E".
			// Results in [10, 12) "A", [12, 18) "E", [18, 20) "A".
			map.insert( 12, 18, "E" );
			THEN( "The interval is split" )
			{
				CHECK( map.size() == 4 ); // [10, 12) "A", [12, 18) "E", [18, 20) "A", [30, 40) "B"
				CHECK( map.find( 11 ) == "A" );
				CHECK( map.find( 15 ) == "E" );
				CHECK( map.find( 19 ) == "A" );
			}
		}
	}
}

SCENARIO( "IntervalMap erase operations" )
{
	GIVEN( "An IntervalMap with data" )
	{
		IntervalMap<int, std::string> map;
		map.insert( 10, 20, "A" );
		map.insert( 30, 40, "B" );

		WHEN( "Erasing a non-overlapping range" )
		{
			map.erase( 20, 30 );
			THEN( "Nothing changes" )
			{
				CHECK( map.size() == 2 );
				CHECK( map.contains( 15 ) );
				CHECK( map.contains( 35 ) );
			}
		}

		WHEN( "Erasing a range that fully covers an interval" )
		{
			map.erase( 5, 25 );
			THEN( "The interval is removed" )
			{
				CHECK( map.size() == 1 );
				CHECK( !map.contains( 15 ) );
				CHECK( map.contains( 35 ) );
			}
		}

		WHEN( "Erasing a range that partially overlaps (splitting/trimming)" )
		{
			// [10, 20) -> "A". Erase [12, 18).
			// Should leave [10, 12) and [18, 20).
			map.erase( 12, 18 );
			THEN( "The interval is trimmed/split" )
			{
				CHECK( map.size() == 3 ); // [10, 12), [18, 20), [30, 40)
				CHECK( map.find( 11 ) == "A" );
				CHECK( !map.find( 15 ).has_value() );
				CHECK( map.find( 19 ) == "A" );
				CHECK( map.find( 35 ) == "B" );
			}
		}
	}
}

SCENARIO( "IntervalMap edge cases" )
{
	IntervalMap<int, int> map;

	SECTION( "Inserting empty or invalid range" )
	{
		CHECK_THROWS_AS( map.insert( 20, 10, 1 ), std::invalid_argument );
		CHECK_THROWS_AS( map.insert( 10, 10, 1 ), std::invalid_argument );
	}

	SECTION( "Erasing empty or invalid range" )
	{
		map.insert( 10, 20, 1 );
		map.erase( 20, 10 );
		CHECK( map.size() == 1 );
		map.erase( 10, 10 );
		CHECK( map.size() == 1 );
	}

	SECTION( "Clear" )
	{
		map.insert( 10, 20, 1 );
		map.clear();
		CHECK( map.empty() );
	}
}
