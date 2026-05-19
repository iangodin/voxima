#include <catch2/catch_test_macros.hpp>
#include <visuals/CreateStyledText.h>
#include <visuals/StyledText.h>
#include <core/Stage.h>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>

TEST_CASE( "CreateStyledText basic rendering", "[visuals]" )
{
	Stage stage;
	auto theme = AcademicClassic::create_theme();
	stage.theme = &theme;

	StyledText text( "Hello World" );
	text.set_color( 0, 5, { 1.0f, 0.0f, 0.0f, 1.0f } ); // "Hello" in red

	SECTION( "It can create a group with glyphs" )
	{
		auto &group = create_styled_text( stage, text );
		CHECK( group.size() > 0 );

		// Basic check that we have some glyphs
		// "Hello World" has 11 characters including space
		// HarfBuzz might generate different number of glyphs, but should be around 11
		CHECK( group.size() >= 10 );
	}

	SECTION( "It handles multiple lines" )
	{
		StyledText multiline( "Hello\nWorld" );
		auto &group = create_styled_text( stage, multiline );
		CHECK( group.size() >= 10 );
	}
}
