#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <themes/NewComputerModern.h>
#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
#include <visuals/MissingGlyph.h>
#include <visuals/Outline.h>
#include <visuals/StyledText.h>
#include <utils/Utf8.h>
#include <visuals/Tags.h>

#include <algorithm>

using Catch::Approx;

namespace
{
constexpr char32_t ABSENT = 0x10FFFD; // a private-use character no font here has

struct Fixture
{
	Theme theme = NewComputerModern::create_theme();
	FontStack stack = theme.get_stack( FontStyle{ Intent::PROSE } );

	Fixture()
	{
		clear_missing_glyph_reports();
	}
	~Fixture()
	{
		clear_missing_glyph_reports();
	}

	std::unique_ptr<Group> text( const std::string &s )
	{
		return make_text( theme, s, FontStyle{ Intent::PROSE } );
	}
};

std::string utf8( char32_t cp )
{
	std::string out;
	append_utf8( out, cp );
	return out;
}

bool has_tag( const OutlineShape &shape, const char *name )
{
	return std::find( shape.tags.begin(), shape.tags.end(), name ) != shape.tags.end();
}
}

TEST_CASE( "FontStack finds a character, or says no font has it", "[glyphs]" )
{
	Fixture f;

	const auto a = f.stack.find_codepoint( U'A' );
	REQUIRE( a.has_value() );
	CHECK( a->font_index == 0 );
	CHECK( a->glyph_index != 0 );
	CHECK_FALSE( f.stack.font( a->font_index ).notdef );

	SECTION( "the catch-all font at the end of a theme's stack has everything, and says so" )
	{
		const auto entry = f.stack.find_codepoint( ABSENT );
		REQUIRE( entry.has_value() );
		CHECK( f.stack.font( entry->font_index ).notdef );
		CHECK( entry->font_index == f.stack.size() - 1 );
	}

	SECTION( "without it, nothing has it" )
	{
		FontStack bare;
		bare.append( f.stack.font( 0 ) );
		CHECK_FALSE( bare.find_codepoint( ABSENT ).has_value() );
		CHECK_THROWS_AS( bare.lookup_codepoint( ABSENT ), std::runtime_error );
		CHECK( bare.lookup_codepoint( U'A' ).font_index == 0 );
	}
}

TEST_CASE( "needs_marker: absent characters and the replacement character", "[glyphs]" )
{
	Fixture f;
	CHECK_FALSE( needs_marker( f.stack, U'A' ) );
	CHECK_FALSE( needs_marker( f.stack, U'\u00E9' ) );
	CHECK_FALSE( needs_marker( f.stack, 0x1F600 ) ); // an emoji: a real font has it
	// only the catch-all font (which draws a box for anything) has it: really missing
	CHECK( needs_marker( f.stack, ABSENT ) );
	// even if a font has a glyph for it, it is the sign that something went wrong
	CHECK( needs_marker( f.stack, REPLACEMENT_CHARACTER ) );

	FontStack bare;
	bare.append( f.stack.font( 0 ) );
	CHECK( needs_marker( bare, ABSENT ) ); // and with no catch-all at all
	CHECK_FALSE( needs_marker( bare, U'A' ) );
}

TEST_CASE( "create_ranges gives markers a range of their own", "[glyphs]" )
{
	Fixture f;

	SECTION( "an absent character between letters" )
	{
		const std::string s = "a" + utf8( ABSENT ) + "b";
		const auto ranges = create_ranges( s, f.stack );
		REQUIRE( ranges.size() == 3 );
		CHECK( ranges[0].font_index == 0 );
		CHECK( ranges[1].font_index == TextRange::MISSING );
		CHECK( ranges[1].start == 1 );
		CHECK( ranges[1].length == 4 );
		CHECK( ranges[2].font_index == 0 );
	}

	SECTION( "the replacement character, whatever the fonts have" )
	{
		const auto ranges = create_ranges( "x\xEF\xBF\xBDy", f.stack );
		REQUIRE( ranges.size() == 3 );
		CHECK( ranges[1].font_index == TextRange::MISSING );
	}

	SECTION( "neighbours share a range" )
	{
		const std::string s = utf8( ABSENT ) + utf8( 0x10FFFE ) + "\xEF\xBF\xBD";
		const auto ranges = create_ranges( s, f.stack );
		REQUIRE( ranges.size() == 1 );
		CHECK( ranges[0].font_index == TextRange::MISSING );
		CHECK( ranges[0].length == s.size() );
	}

	SECTION( "text with nothing missing is as before" )
	{
		const auto ranges = create_ranges( "Hello", f.stack );
		REQUIRE( ranges.size() == 1 );
		CHECK( ranges[0].font_index == 0 );
	}
}

TEST_CASE( "the marker is a magenta frame with the code in it", "[glyphs]" )
{
	Fixture f;
	const FontFace &font = f.stack.font( 0 );
	const auto marker = create_missing_glyph( REPLACEMENT_CHARACTER, font );

	CHECK( marker->color == Color{ 1.F, 0.F, 1.F, 1.F } );
	CHECK( std::find( marker->tags.begin(), marker->tags.end(), MISSING_GLYPH_TAG ) != marker->tags.end() );
	CHECK( marker->advanceX > 0.F );
	REQUIRE( marker->extent.has_value() );
	CHECK( marker->extent->width() == Approx( marker->advanceX ) );

	const auto shapes = to_outline( *marker );
	REQUIRE( shapes.size() == 1 );
	const auto &contours = shapes[0].contours;

	// the frame and its hole, then the letters of FFFD (the D has a hole of its own)
	REQUIRE( contours.size() >= 6 );
	CHECK( signed_area( contours[0] ) * signed_area( contours[1] ) < 0.0 ); // opposite windings: a hole
	CHECK( std::abs( signed_area( contours[0] ) ) > std::abs( signed_area( contours[1] ) ) );

	SECTION( "and the code sits inside the frame" )
	{
		const AABB inner = on_curve_bounds( contours[1] );
		for ( size_t i = 2; i < contours.size(); ++i )
		{
			const AABB box = on_curve_bounds( contours[i] );
			CHECK( box.min.x() >= inner.min.x() - 1e-3F );
			CHECK( box.max.x() <= inner.max.x() + 1e-3F );
			CHECK( box.min.y() >= inner.min.y() - 1e-3F );
			CHECK( box.max.y() <= inner.max.y() + 1e-3F );
		}
	}

	SECTION( "a longer code makes a wider box" )
	{
		const auto longer = create_missing_glyph( ABSENT, font ); // 10FFFD holds FFFD and more
		CHECK( longer->advanceX > marker->advanceX );
	}
}

TEST_CASE( "text with a missing character lays out and marks it", "[glyphs]" )
{
	Fixture f;
	const std::string s = "a" + utf8( ABSENT ) + "b";
	auto root = f.text( s ); // once, this threw and ended the render

	const auto shapes = to_outline( *root );
	REQUIRE( shapes.size() == 3 );
	CHECK_FALSE( has_tag( shapes[0], MISSING_GLYPH_TAG ) );
	CHECK( has_tag( shapes[1], MISSING_GLYPH_TAG ) );
	CHECK_FALSE( has_tag( shapes[2], MISSING_GLYPH_TAG ) );
	CHECK( shapes[1].fill == Color{ 1.F, 0.F, 1.F, 1.F } );

	SECTION( "it takes its place in the line: nothing overlaps it" )
	{
		const AABB before = on_curve_bounds( shapes[0].contours[0] );
		AABB marker;
		for ( const auto &c: shapes[1].contours ) marker.add( on_curve_bounds( c ) );
		const AABB after = on_curve_bounds( shapes[2].contours[0] );
		CHECK( marker.min.x() >= before.max.x() );
		CHECK( after.min.x() >= marker.max.x() );
	}

	SECTION( "and stays part of its word" )
	{
		Visual *word = find_word( *root, 0 );
		REQUIRE( word != nullptr );
		CHECK( static_cast<Group *>( word )->size() == 3 );
		CHECK( find_word( *root, 1 ) == nullptr );
	}

	SECTION( "and is recorded" )
	{
		REQUIRE( missing_glyph_reports().size() == 1 );
		CHECK( missing_glyph_reports()[0].codepoint == ABSENT );
		CHECK( missing_glyph_reports()[0].count == 1 );
		CHECK( missing_glyph_reports()[0].context == s );
		CHECK( describe_missing_glyphs().find( "U+10FFFD" ) != std::string::npos );
	}
}

TEST_CASE( "the replacement character is marked even though fonts draw it", "[glyphs]" )
{
	Fixture f;
	auto root = f.text( "not found: \xEF\xBF\xBD" );

	const auto shapes = to_outline( *root );
	REQUIRE( !shapes.empty() );
	CHECK( has_tag( shapes.back(), MISSING_GLYPH_TAG ) );
	REQUIRE( missing_glyph_reports().size() == 1 );
	CHECK( missing_glyph_reports()[0].codepoint == REPLACEMENT_CHARACTER );
}

TEST_CASE( "a style's color does not hide a marker", "[glyphs]" )
{
	Fixture f;
	StyledText styled( "a" + utf8( ABSENT ) + "b" );
	styled.set_color( 0, styled.size(), Color{ 0.F, 1.F, 0.F, 1.F } );
	auto root = make_styled_text( f.theme, styled );

	const auto shapes = to_outline( *root );
	REQUIRE( shapes.size() == 3 );
	CHECK( shapes[0].fill == Color{ 0.F, 1.F, 0.F, 1.F } );
	CHECK( shapes[1].fill == Color{ 1.F, 0.F, 1.F, 1.F } );
}

TEST_CASE( "text without missing characters records nothing", "[glyphs]" )
{
	Fixture f;
	auto root = f.text( "Hello, world" );
	CHECK( missing_glyph_reports().empty() );
	CHECK( describe_missing_glyphs().empty() );
}

TEST_CASE( "the record counts repeats and lists each place once", "[glyphs]" )
{
	clear_missing_glyph_reports();

	report_missing_glyph( 0x1F600, "one" );
	report_missing_glyph( 0x1F600, "one" );
	report_missing_glyph( 0x1F600, "two" );
	report_missing_glyph( 0xFFFD, "one" );

	REQUIRE( missing_glyph_reports().size() == 3 );
	CHECK( missing_glyph_reports()[0].count == 2 );
	CHECK( missing_glyph_reports()[1].count == 1 );

	const std::string text = describe_missing_glyphs();
	CHECK( text.find( "4 missing glyphs" ) != std::string::npos );
	CHECK( text.find( "U+1F600 x2  in \"one\"" ) != std::string::npos );
	CHECK( text.find( "U+FFFD  in \"one\"" ) != std::string::npos );

	SECTION( "a long text is cut short" )
	{
		clear_missing_glyph_reports();
		report_missing_glyph( 0x41, std::string( 200, 'x' ) );
		const std::string cut = describe_missing_glyphs();
		CHECK( cut.find( "..." ) != std::string::npos );
		CHECK( cut.size() < 200 );
	}

	SECTION( "one is singular" )
	{
		clear_missing_glyph_reports();
		report_missing_glyph( 0x41, "x" );
		CHECK( describe_missing_glyphs().find( "1 missing glyph " ) != std::string::npos );
	}

	clear_missing_glyph_reports();
	CHECK( missing_glyph_reports().empty() );
}
