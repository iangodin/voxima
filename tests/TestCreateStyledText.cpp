#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
#include <visuals/StyledText.h>
#include <visuals/Glyph.h>
#include <utils/Words.h>
#include <visuals/Tags.h>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>

namespace
{
// Glyphs anywhere below `visual` (lines contain word groups, which contain glyphs)
size_t glyph_count( const Visual &visual )
{
	if ( dynamic_cast<const Glyph *>( &visual ) )
	{
		return 1;
	}
	size_t count = 0;
	if ( auto *group = dynamic_cast<const Group *>( &visual ) )
	{
		for ( const auto &child: *group )
		{
			count += glyph_count( *child );
		}
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

namespace
{
const Group &line_of( const Group &root, size_t index )
{
	return static_cast<const Group &>( **( root.begin() + index ) );
}

// The visuals directly in a line that are word groups, in order
std::vector<const Group *> word_groups( const Group &line )
{
	std::vector<const Group *> out;
	for ( const auto &child: line )
	{
		if ( auto *group = dynamic_cast<const Group *>( child.get() ) )
		{
			out.push_back( group );
		}
	}
	return out;
}
}

TEST_CASE( "make_styled_text groups each word's glyphs and tags them", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();

	SECTION( "words become tagged groups, spaces stay in the line" )
	{
		StyledText text( "Hello big world" );
		auto root = make_styled_text( theme, text );

		auto words = word_groups( line_of( *root, 0 ) );
		REQUIRE( words.size() == 3 );
		CHECK( words[0]->tags == std::vector<std::string>{ "word0" } );
		CHECK( words[2]->tags == std::vector<std::string>{ "word2" } );
		CHECK( words[0]->size() == 5 );
		CHECK( words[1]->size() == 3 );
		CHECK( words[2]->size() == 5 );
		// two space glyphs sit directly in the line
		CHECK( line_of( *root, 0 ).size() == 3 + 2 );
		CHECK( glyph_count( *root ) == 15 );
	}

	SECTION( "word groups sit at the origin so glyph positions are unchanged" )
	{
		StyledText text( "ab cd" );
		auto root = make_styled_text( theme, text );

		for ( auto *word: word_groups( line_of( *root, 0 ) ) )
		{
			CHECK( word->position.x() == 0.F );
			CHECK( word->position.y() == 0.F );
		}
		// the second word starts to the right of the first
		auto words = word_groups( line_of( *root, 0 ) );
		REQUIRE( words.size() == 2 );
		auto &last_of_first = **( words[0]->begin() + 1 );
		auto &first_of_second = **words[1]->begin();
		CHECK( first_of_second.position.x() > last_of_first.position.x() );
	}

	SECTION( "tags count on through lines" )
	{
		StyledText text( "one two\nthree" );
		auto root = make_styled_text( theme, text );

		CHECK( word_groups( line_of( *root, 0 ) ).size() == 2 );
		auto second = word_groups( line_of( *root, 1 ) );
		REQUIRE( second.size() == 1 );
		CHECK( second[0]->tags == std::vector<std::string>{ "word2" } );
		CHECK( find_tag( *root, "word2" ) == second[0] );
	}

	SECTION( "find_word looks a word up by index" )
	{
		StyledText text( "one two\nthree" );
		auto root = make_styled_text( theme, text );

		auto *two = find_word( *root, 1 );
		REQUIRE( two != nullptr );
		CHECK( glyph_count( *two ) == 3 );
		CHECK( glyph_count( *find_word( *root, 2 ) ) == 5 );
		CHECK( find_word( *root, 3 ) == nullptr );
	}

	SECTION( "words agree with split_words on punctuation" )
	{
		StyledText text( "Wait \u2014 what, really?" );
		auto root = make_styled_text( theme, text );

		auto words = word_groups( line_of( *root, 0 ) );
		REQUIRE( words.size() == split_words( text.text() ).size() );
		REQUIRE( words.size() == 3 );
		CHECK( words[1]->size() == 5 ); // "what,"
	}

	SECTION( "an RTL word is gathered by logical position" )
	{
		StyledText rtl( "\u0645\u0631\u062D\u0628\u0627 \u0645\u0631\u062D\u0628\u0627" );
		rtl.set_bidi_level( 0, rtl.size(), 1 );
		auto root = make_styled_text( theme, rtl );

		CHECK( word_groups( line_of( *root, 0 ) ).size() == 2 );
	}

	SECTION( "math alphabet mapping keeps words intact" )
	{
		auto root = make_text( theme, "x y", FontStyle{ Intent::MATH } );
		auto words = word_groups( line_of( *root, 0 ) );
		REQUIRE( words.size() == 2 );
		CHECK( words[0]->size() == 1 );
		CHECK( words[1]->size() == 1 );
	}

	SECTION( "styling ranges that split a word do not split its group" )
	{
		StyledText text( "Hello" );
		text.set_color( 0, 2, { 1.F, 0.F, 0.F, 1.F } );
		auto root = make_styled_text( theme, text );

		auto words = word_groups( line_of( *root, 0 ) );
		REQUIRE( words.size() == 1 );
		CHECK( words[0]->size() == 5 );
	}
}

namespace
{
float baseline_gap( const Group &root )
{
	return static_cast<const Group &>( **root.begin() ).baseline -
		   static_cast<const Group &>( **( root.begin() + 1 ) ).baseline;
}
}

TEST_CASE( "make_styled_text line spacing follows the font size", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();

	SECTION( "bigger text gets more room between lines" )
	{
		auto normal = make_text( theme, "Hello\nWorld", FontStyle{ Intent::PROSE } );
		auto big = make_text( theme, "Hello\nWorld", FontStyle{ Intent::PROSE }.large() );
		CHECK( baseline_gap( *big ) > baseline_gap( *normal ) );
	}

	SECTION( "line_spacing scales the gap" )
	{
		auto tight = make_text( theme, "Hello\nWorld", FontStyle{ Intent::PROSE }, std::nullopt, TextLayout{ .line_spacing = 1.0F } );
		auto loose = make_text( theme, "Hello\nWorld", FontStyle{ Intent::PROSE }, std::nullopt, TextLayout{ .line_spacing = 2.0F } );
		CHECK( baseline_gap( *loose ) == Catch::Approx( 2.0F * baseline_gap( *tight ) ) );
	}

	SECTION( "the default leaves lines apart, not on top of each other" )
	{
		auto text = make_text( theme, "Hello\nWorld", FontStyle{ Intent::PROSE } );
		// gap is 1.2x the font size, so more than one em
		CHECK( baseline_gap( *text ) > 0.F );
	}

	SECTION( "a blank line takes the spacing of the line before it" )
	{
		auto text = make_text( theme, "Hello\n\nWorld", FontStyle{ Intent::PROSE } );
		REQUIRE( text->size() == 3 );
		const float first = static_cast<const Group &>( **text->begin() ).baseline;
		const float second = static_cast<const Group &>( **( text->begin() + 1 ) ).baseline;
		const float third = static_cast<const Group &>( **( text->begin() + 2 ) ).baseline;
		CHECK( ( first - second ) == Catch::Approx( second - third ) );
	}
}

TEST_CASE( "make_styled_text aligns lines within the widest one", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();
	const std::string text = "A much longer first line\nab";
	const FontStyle style{ Intent::PROSE };

	auto offset = [&]( TextAlign align, size_t line )
	{
		auto root = make_text( theme, text, style, std::nullopt, TextLayout{ .align = align } );
		return static_cast<const Group &>( **( root->begin() + line ) ).position.x();
	};

	SECTION( "left leaves every line at the origin" )
	{
		CHECK( offset( TextAlign::LEFT, 0 ) == 0.F );
		CHECK( offset( TextAlign::LEFT, 1 ) == 0.F );
	}

	SECTION( "the widest line stays put, shorter ones move right" )
	{
		CHECK( offset( TextAlign::CENTER, 0 ) == 0.F );
		CHECK( offset( TextAlign::CENTER, 1 ) > 0.F );
		CHECK( offset( TextAlign::RIGHT, 0 ) == 0.F );
	}

	SECTION( "right moves a short line twice as far as center" )
	{
		CHECK( offset( TextAlign::RIGHT, 1 ) == Catch::Approx( 2.0F * offset( TextAlign::CENTER, 1 ) ) );
	}

	SECTION( "alignment does not change the number of glyphs or words" )
	{
		auto left = make_text( theme, text, style );
		auto centered = make_text( theme, text, style, std::nullopt, TextLayout{ .align = TextAlign::CENTER } );
		CHECK( glyph_count( *left ) == glyph_count( *centered ) );
		CHECK( find_word( *centered, 5 ) != nullptr );
	}
}
