
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

SCENARIO( "StyledText style application" )
{
	GIVEN( "a styled text" )
	{
		StyledText text( "Hello World" );

		WHEN( "setting font style" )
		{
			FontStyle style;
			style.bold().italic().set_intent( "title" );
			text.set_font_style( 0, 5, style );

			THEN( "styles should be correctly applied" )
			{
				auto styles = text.styles().intervals();
				// "Hello World" (11 chars) has line=0.
				// set_font_style(0, 5) splits it into [0, 5) and [5, 11).
				REQUIRE( styles.size() == 2 );
				CHECK( styles[0].start == 0 );
				CHECK( styles[0].end == 5 );
				CHECK( styles[0].value.font_style == style );
				CHECK( styles[0].value.line == 0 );

				CHECK( styles[1].start == 5 );
				CHECK( styles[1].end == 11 );
				CHECK( styles[1].value.line == 0 );
			}
		}

		WHEN( "checking color" )
		{
			Color red = { 1.0f, 0.0f, 0.0f, 1.0f };
			text.set_color( 6, 11, red );

			THEN( "color should be correctly applied" )
			{
				auto styles = text.styles().intervals();
				// IntervalMap might have gaps or previous styles
				// In this case, since it's the only style, it should be at index 0 (if sorted and started at 0)
				// but wait, Hello World is 11 chars. set_color(6, 11) is the second half.
				// Wait, now we have line indices, so we might have more intervals.
				
				// Find interval at index 6
				auto style = text.styles().find( 6 );
				REQUIRE( style.has_value() );
				REQUIRE( style->color.has_value() );
				CHECK( style->color.value() == red );
			}
		}

		WHEN( "checking line indices" )
		{
			StyledText multiline( "abc\ndef" );

			THEN( "line indices should be set" )
			{
				auto style0 = multiline.styles().find( 0 ); // 'a'
				REQUIRE( style0.has_value() );
				CHECK( style0->line == 0 );

				auto style_nl = multiline.styles().find( 3 ); // '\n'
				REQUIRE( style_nl.has_value() );
				CHECK( style_nl->line == 0 );

				auto style1 = multiline.styles().find( 4 ); // 'd'
				REQUIRE( style1.has_value() );
				CHECK( style1->line == 1 );
			}
		}

		WHEN( "setting a language on part of the text" )
		{
			text.set_language( 6, 11, "fr-CA" );

			THEN( "language should be set only on that range" )
			{
				auto unset = text.styles().find( 0 );
				REQUIRE( unset.has_value() );
				CHECK_FALSE( unset->language.has_value() );

				auto style = text.styles().find( 6 );
				REQUIRE( style.has_value() );
				REQUIRE( style->language.has_value() );
				CHECK( style->language.value() == "fr-CA" );
			}
		}
	}
}

namespace
{
std::vector<size_t> bidi_levels( const StyledText &text )
{
	std::vector<size_t> levels;
	text.styles().for_each(
		0,
		text.size(),
		TextStyle{},
		[&]( size_t start, size_t end, const TextStyle &style )
		{
			for ( size_t i = start; i < end; ++i )
			{
				levels.push_back( style.bidi_level.value_or( 99 ) );
			}
		} );
	return levels;
}
}

SCENARIO( "StyledText bidi analysis" )
{
	GIVEN( "a string of digits and punctuation" )
	{
		StyledText text( "3.141592" );

		THEN( "every character is left-to-right (level 0) in a single run" )
		{
			auto levels = bidi_levels( text );
			REQUIRE( levels.size() == text.size() );
			for ( auto level: levels )
			{
				CHECK( level == 0 );
			}
			CHECK( text.styles().size() == 1 );
		}
	}

	GIVEN( "Latin text" )
	{
		StyledText text( "Hello World" );

		THEN( "every character is level 0" )
		{
			for ( auto level: bidi_levels( text ) )
			{
				CHECK( level == 0 );
			}
		}
	}

	GIVEN( "Arabic text" )
	{
		StyledText text( "مرحبا" );

		THEN( "every character is right-to-left (level 1)" )
		{
			auto levels = bidi_levels( text );
			REQUIRE_FALSE( levels.empty() );
			for ( auto level: levels )
			{
				CHECK( level == 1 );
			}
		}
	}
}
