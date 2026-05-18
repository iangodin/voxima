
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
