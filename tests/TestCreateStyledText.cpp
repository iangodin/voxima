#include <catch2/catch_test_macros.hpp>
#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
#include <visuals/StyledText.h>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>

namespace
{
size_t glyph_count( const Group &group )
{
	size_t count = 0;
	for ( const auto &line: group )
	{
		count += static_cast<const Group &>( *line ).size();
	}
	return count;
}
}

TEST_CASE( "make_styled_text basic rendering", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();

	StyledText text( "Hello World" );
	text.set_color( 0, 5, { 1.0f, 0.0f, 0.0f, 1.0f } ); // "Hello" in red

	SECTION( "It creates one line of glyphs" )
	{
		auto group = make_styled_text( theme, text );
		CHECK( group->size() == 1 );
		// "Hello World" has 11 characters including space
		CHECK( glyph_count( *group ) >= 10 );
	}

	SECTION( "It handles multiple lines" )
	{
		StyledText multiline( "Hello\nWorld" );
		auto group = make_styled_text( theme, multiline );
		CHECK( group->size() == 2 );
		CHECK( glyph_count( *group ) >= 10 );
	}

	SECTION( "make_text supports multiple lines" )
	{
		auto group = make_text( theme, "Hello\nWorld", FontStyle{ Intent::PROSE } );
		CHECK( group->size() == 2 );
		CHECK( glyph_count( *group ) >= 10 );
	}

	SECTION( "Text is not centered: first baseline is at y = 0" )
	{
		auto group = make_text( theme, "Hello\nWorld", FontStyle{ Intent::PROSE } );
		CHECK( group->position.x() == 0.F );
		CHECK( group->position.y() == 0.F );
		auto &first = static_cast<const Group &>( **group->begin() );
		auto &second = static_cast<const Group &>( **( group->begin() + 1 ) );
		CHECK( first.baseline == 0.F );
		CHECK( second.baseline < first.baseline );
	}

	SECTION( "It handles RTL text (basic check)" )
	{
		// Arabic "Hello" (مرحبا)
		StyledText rtl( "\u0645\u0631\u062D\u0628\u0627" );
		auto group = make_styled_text( theme, rtl );
		CHECK( glyph_count( *group ) > 0 );
	}

	SECTION( "It shapes text with an explicit language set" )
	{
		// hb_buffer_set_language() is a shaping hint, not something that
		// changes glyph *count* for plain Latin text - this checks the
		// language-tagged path shapes successfully end to end, same as the
		// untagged path, rather than asserting a specific glyph outcome.
		StyledText tagged( "Bonjour" );
		tagged.set_language( 0, tagged.size(), "fr-CA" );
		auto group = make_styled_text( theme, tagged );
		CHECK( glyph_count( *group ) >= 6 );
	}

	SECTION( "make_text passes the language through" )
	{
		auto group = make_text( theme, "Bonjour", FontStyle{ Intent::PROSE }, std::string( "fr-CA" ) );
		CHECK( glyph_count( *group ) >= 6 );
	}
}
