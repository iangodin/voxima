
#include <catch2/catch_test_macros.hpp>

#include <visuals/StyledText.h>

SCENARIO( "StyledText" )
{
	GIVEN( "an empty/default text" )
	{
		StyledText text;

		WHEN( "looking at lines" )
		{
			THEN( "its all empty" )
			{
				CHECK( text.line_count() == 0 );
				CHECK_FALSE( text.index( 0, 0 ).has_value() );
				CHECK( text.lines().empty() );
			}
		}
	}
}

SCENARIO( "StyledText constructor with string" )
{
	GIVEN( "a string with single line" )
	{
		StyledText text( "hello" );

		WHEN( "checking line count" )
		{
			THEN( "should be 1" )
			{
				CHECK( text.line_count() == 1 );
			}
		}

		WHEN( "getting the line" )
		{
			THEN( "should return the original string" )
			{
				REQUIRE( text.line_count() >= 1 );
				CHECK( text.line( 0 ) == "hello" );
			}
		}
	}
}

SCENARIO( "StyledText constructor with newlines" )
{
	GIVEN( "a string with multiple lines" )
	{
		StyledText text( "hello\nworld" );

		WHEN( "checking line count" )
		{
			THEN( "should be 2" )
			{
				CHECK( text.line_count() == 2 );
			}
		}

		WHEN( "getting first line" )
		{
			THEN( "should return first line" )
			{
				REQUIRE( text.line_count() >= 1 );
				CHECK( text.line( 0 ) == "hello" );
			}
		}

		WHEN( "getting second line" )
		{
			THEN( "should return second line" )
			{
				REQUIRE( text.line_count() >= 2 );
				CHECK( text.line( 1 ) == "world" );
			}
		}
	}
}

SCENARIO( "StyledText index helper" )
{
	GIVEN( "a simple text" )
	{
		StyledText text( "hello" );

		WHEN( "getting index at start" )
		{
			THEN( "should return 0" )
			{
				auto result = text.index( 0, 0 );
				REQUIRE( result.has_value() );
				CHECK( result.value() == 0 );
			}
		}

		WHEN( "getting index at middle" )
		{
			THEN( "should return 2" )
			{
				auto result = text.index( 0, 2 );
				REQUIRE( result.has_value() );
				CHECK( result.value() == 2 );
			}
		}

		WHEN( "getting index at end" )
		{
			THEN( "should return 4" )
			{
				auto result = text.index( 0, 4 );
				REQUIRE( result.has_value() );
				CHECK( result.value() == 4 );
			}
		}

		WHEN( "getting index beyond line" )
		{
			THEN( "should return nullopt" )
			{
				auto result = text.index( 0, 10 );
				REQUIRE_FALSE( result.has_value() );
			}
		}

		WHEN( "getting index on invalid line" )
		{
			THEN( "should return nullopt" )
			{
				auto result = text.index( 999, 0 );
				REQUIRE_FALSE( result.has_value() );
			}
		}
	}
}

SCENARIO( "StyledText lines access" )
{
	GIVEN( "an empty text" )
	{
		StyledText text;

		WHEN( "checking lines" )
		{
			THEN( "should be empty" )
			{
				CHECK( text.lines().empty() );
			}
		}
	}

	GIVEN( "a text with multiple lines" )
	{
		StyledText text( "hello\nworld" );
		REQUIRE( text.lines().size() == 2 );

		WHEN( "checking lines" )
		{
			THEN( "should have 2 lines" )
			{
				CHECK( text.lines().size() == 2 );
			}
		}

		WHEN( "checking first line" )
		{
			THEN( "should be hello" )
			{
				CHECK( text.lines()[0] == "hello" );
			}
		}

		WHEN( "checking second line" )
		{
			THEN( "should be world" )
			{
				CHECK( text.lines()[1] == "world" );
			}
		}
	}
}
