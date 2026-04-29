
#include <catch2/catch_test_macros.hpp>

#include <Theme.h>
#include <CreateText.h>
#include <NewComputerModern.h>

SCENARIO( "Text splitting" )
{
	Theme theme = NewComputerModern::create_theme();
	FontStack font = theme.get_stack( TextStyle{ "prose" } );

	GIVEN( "a string with fallback characters" )
	{
		std::string message = "Hello 😀";

		WHEN( "creating ranges" )
		{
			std::vector<TextRange> ranges = create_ranges( message, font );

			THEN( "two ranges are created" )
			{
				REQUIRE( ranges.size() == 2 );
				CHECK( ranges[0].font_index == 0 );
				CHECK( ranges[0].start == 0 );
				CHECK( ranges[0].length == 6 );

				CHECK( ranges[1].font_index == 1 );
				CHECK( ranges[1].start == 6 );
				CHECK( ranges[1].length == 4 );
			}
		}

	}
}
