#include <catch2/catch_test_macros.hpp>
#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
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

	SECTION( "create_text supports multiple lines now" )
	{
		auto &group = create_text( stage, "Hello\nWorld", FontStyle{ Intent::PROSE } );
		CHECK( group.size() >= 10 );
	}

	SECTION( "It handles RTL text (basic check)" )
	{
		// Arabic "Hello" (مرحبا)
		StyledText rtl( "\u0645\u0631\u062D\u0628\u0627" );
		auto &group = create_styled_text( stage, rtl );
		CHECK( group.size() > 0 );
	}
}
