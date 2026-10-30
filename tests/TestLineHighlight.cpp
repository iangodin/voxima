#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <code/LineHighlight.h>
#include <core/CameraMove.h>
#include <core/Theme.h>
#include <exporters/SVGWriter.h>
#include <themes/NewComputerModern.h>
#include <visuals/Rectangle.h>

#include <sstream>

using Catch::Approx;
using namespace std::chrono_literals;

TEST_CASE( "Lines are numbered from 1, in runs that never touch", "[highlight]" )
{
	CHECK( Lines().empty() );
	CHECK( Lines( 5 ).contains( 5 ) );
	CHECK_FALSE( Lines( 5 ).contains( 4 ) );
	CHECK_FALSE( Lines( 5 ).contains( 6 ) );
	CHECK( Lines( 9, 13 ).contains( 11 ) );
	CHECK( Lines( 13, 9 ) == Lines( 9, 13 ) ); // either way round

	// Several ranges, in any order; the ones that touch or overlap are one
	const Lines several{ { 9, 13 }, { 3, 4 }, { 5, 6 }, { 12, 15 } };
	REQUIRE( several.runs().size() == 2 );
	CHECK( several.runs()[0] == std::pair<size_t, size_t>{ 3, 6 } );
	CHECK( several.runs()[1] == std::pair<size_t, size_t>{ 9, 15 } );
	CHECK( several.contains( 6 ) );
	CHECK_FALSE( several.contains( 7 ) );
	CHECK( several.contains( 15 ) );

	// Built up
	CHECK( Lines( 3 ).add( 7, 8 ).add( 12 ).runs().size() == 3 );

	// There is no line 0
	CHECK( Lines( 0 ).empty() );
	CHECK( Lines( 0, 2 ) == Lines( 1, 2 ) );
}

namespace
{
const char *SOURCE = "int a;\nint b;\nint c;\nint d;\nint e;\nint f;";

float alpha_of( CodeListing &listing, size_t line )
{
	return listing.line( line ).color.a;
}

// Where the bands are, top and bottom, for the ones that can be seen
std::vector<std::pair<float, float>> bands_of( CodeListing &listing )
{
	std::vector<std::pair<float, float>> out;
	for ( const auto &child: *listing.background )
	{
		const auto &band = dynamic_cast<const Rectangle &>( *child );
		if ( band.color.a > 0.F )
		{
			out.emplace_back( band.position.y() + band.half_height, band.position.y() - band.half_height );
		}
	}
	return out;
}
}

TEST_CASE( "A highlight dims the other lines, and none dims nothing", "[highlight]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::C, SOURCE );
	REQUIRE( listing->line_count() == 6 );

	LineHighlight pointer( *listing, theme );
	for ( size_t i = 0; i < 6; ++i )
	{
		CHECK( alpha_of( *listing, i ) == 1.F ); // to begin with, nothing is pointed out
	}
	CHECK( bands_of( *listing ).empty() );

	pointer.set( Lines( 2, 3 ) ); // lines 2 and 3: the listing's lines 1 and 2
	CHECK( alpha_of( *listing, 0 ) == Approx( 0.35F ) );
	CHECK( alpha_of( *listing, 1 ) == 1.F );
	CHECK( alpha_of( *listing, 2 ) == 1.F );
	CHECK( alpha_of( *listing, 3 ) == Approx( 0.35F ) );

	pointer.set( Lines() );
	CHECK( alpha_of( *listing, 0 ) == 1.F );
	CHECK( bands_of( *listing ).empty() );
}

TEST_CASE( "A highlight puts one band behind each run of lines", "[highlight]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::C, SOURCE );
	LineHighlight pointer( *listing, theme );
	const float pitch = listing->line_pitch;

	// One line: a band a line's pitch tall
	pointer.set( Lines( 2 ) );
	auto bands = bands_of( *listing );
	REQUIRE( bands.size() == 1 );
	CHECK( bands[0].first - bands[0].second == Approx( pitch ) );

	// The band is behind the text of the line: its middle is where the text is (lowercase letters' middle)
	const float baseline = listing->line( 1 ).baseline;
	CHECK( ( bands[0].first + bands[0].second ) / 2.F == Approx( baseline + 0.25F * listing->em ).margin( 1e-4 ) );

	// A range: one band, as tall as its lines together
	pointer.set( Lines( 2, 4 ) );
	bands = bands_of( *listing );
	REQUIRE( bands.size() == 1 );
	CHECK( bands[0].first - bands[0].second == Approx( 3.F * pitch ) );

	// Separate ranges: a band each, with the lines between them not in one
	pointer.set( Lines{ { 1, 2 }, { 5, 6 } } );
	bands = bands_of( *listing );
	REQUIRE( bands.size() == 2 );
	CHECK( bands[0].first - bands[0].second == Approx( 2.F * pitch ) );
	CHECK( bands[1].first - bands[1].second == Approx( 2.F * pitch ) );
	CHECK( bands[0].second > bands[1].first ); // the first is above the second, with a gap
	CHECK( alpha_of( *listing, 2 ) == Approx( 0.35F ) ); // the ones between are dimmed
	CHECK( alpha_of( *listing, 3 ) == Approx( 0.35F ) );

	// The band spans the whole listing, and a little past it
	const Rectangle &band = dynamic_cast<const Rectangle &>( **listing->background->begin() );
	CHECK( band.position.x() - band.half_width < listing->left );
	CHECK( band.position.x() + band.half_width > listing->right );
}

TEST_CASE( "A highlight can be dimming only, and can have a band of its own color", "[highlight]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::C, SOURCE );

	HighlightOptions dimming;
	dimming.bar = false;
	dimming.dim = 0.5F;
	LineHighlight only_dim( *listing, theme, dimming );
	only_dim.set( Lines( 1 ) );
	CHECK( bands_of( *listing ).empty() );
	CHECK( alpha_of( *listing, 4 ) == Approx( 0.5F ) );

	HighlightOptions colored;
	colored.band = Color{ 1.F, 0.F, 0.F, 0.5F };
	LineHighlight red( *listing, theme, colored );
	red.set( Lines( 1 ) );
	const Rectangle &band = dynamic_cast<const Rectangle &>( **listing->background->begin() );
	CHECK( band.color.r == 1.F );
	CHECK( band.color.a == Approx( 0.5F ) );
}

TEST_CASE( "blend moves the band and the dimming from some lines to others", "[highlight]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::C, SOURCE );
	LineHighlight pointer( *listing, theme );
	const float pitch = listing->line_pitch;

	const Lines a( 1 );
	const Lines b( 5 );
	pointer.blend( a, b, 0.5F );

	// The one band, half way down (four lines' pitch between them), and the two lines each half pointed out
	const auto bands = bands_of( *listing );
	REQUIRE( bands.size() == 1 );
	const float top_a = listing->line( 0 ).baseline + 0.25F * listing->em + pitch / 2.F;
	const float top_b = listing->line( 4 ).baseline + 0.25F * listing->em + pitch / 2.F;
	CHECK( bands[0].first == Approx( ( top_a + top_b ) / 2.F ) );
	const float half = 0.35F + 0.65F * 0.5F;
	CHECK( alpha_of( *listing, 0 ) == Approx( half ) );
	CHECK( alpha_of( *listing, 4 ) == Approx( half ) );
	CHECK( alpha_of( *listing, 2 ) == Approx( 0.35F ) );

	// From one run to two: the first slides to the first, and the new one fades in
	pointer.blend( Lines( 1 ), Lines{ { 1, 2 }, { 5, 6 } }, 0.5F );
	auto colors = std::vector<float>();
	for ( const auto &child: *listing->background )
		colors.push_back( dynamic_cast<const Rectangle &>( *child ).color.a );
	REQUIRE( colors.size() == 2 );
	CHECK( colors[0] == Approx( theme.code_palette().line_highlight.a ) );
	CHECK( colors[1] == Approx( theme.code_palette().line_highlight.a * 0.5F ) );
}

TEST_CASE( "to moves the highlight over a length of time, and ends exactly there", "[highlight]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::C, SOURCE );
	LineHighlight pointer( *listing, theme );
	Stage stage;
	stage.camera.frame_rate = 24;

	pointer.set( Lines( 1 ) );
	Coroutine move = pointer.to( stage, Lines( 4, 5 ), 1s );
	move.resume();
	const float start_top = bands_of( *listing )[0].first;
	size_t frames = 1;
	while ( !move.done() )
	{
		move.resume();
		++frames;
	}
	CHECK( frames == 25 );
	CHECK( pointer.lines() == Lines( 4, 5 ) );
	const auto bands = bands_of( *listing );
	REQUIRE( bands.size() == 1 );
	CHECK( bands[0].first < start_top ); // it went down
	CHECK( bands[0].first - bands[0].second == Approx( 2.F * listing->line_pitch ) );
	CHECK( alpha_of( *listing, 0 ) == Approx( 0.35F ) );
	CHECK( alpha_of( *listing, 3 ) == 1.F );
}

TEST_CASE( "A dimmed line's glyphs are drawn fainter in the picture, and the band is behind the text", "[highlight]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::C, "int a;\nint b;" );
	LineHighlight pointer( *listing, theme );
	pointer.set( Lines( 1 ) );

	std::ostringstream body;
	SVGWriter writer( body );
	listing->accept( writer );
	const std::string svg = body.str();

	// The band (a rectangle) is written before any glyph
	const size_t rect = svg.find( "<rect" );
	const size_t glyph = svg.find( "<use" );
	REQUIRE( rect != std::string::npos );
	REQUIRE( glyph != std::string::npos );
	CHECK( rect < glyph );

	// The second line's glyphs are see-through: 0.35 of their color's alpha, the first's are not
	CHECK( svg.find( "fill-opacity=\"0.35" ) != std::string::npos );
}

TEST_CASE( "Lines past the end of the listing are ignored", "[highlight]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto listing = make_code( theme, code::Language::C, SOURCE ); // 6 lines
	LineHighlight pointer( *listing, theme );

	// A range going past the end stops at it
	pointer.set( Lines( 5, 20 ) );
	auto bands = bands_of( *listing );
	REQUIRE( bands.size() == 1 );
	CHECK( bands[0].first - bands[0].second == Approx( 2.F * listing->line_pitch ) ); // lines 5 and 6

	// One wholly past it is not there: with nothing else, it points at nothing, and nothing is dimmed
	pointer.set( Lines( 16 ) );
	CHECK( bands_of( *listing ).empty() );
	for ( size_t i = 0; i < 6; ++i )
	{
		CHECK( alpha_of( *listing, i ) == 1.F );
	}

	// Beside one that is there, it does not band the last line, or dim what the other points at
	pointer.set( Lines{ { 2, 2 }, { 16, 16 } } );
	bands = bands_of( *listing );
	REQUIRE( bands.size() == 1 );
	CHECK( alpha_of( *listing, 1 ) == 1.F );
	CHECK( alpha_of( *listing, 5 ) == Approx( 0.35F ) ); // the last line is dimmed, and not banded
}
