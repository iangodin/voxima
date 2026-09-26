#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <math/Expr.h>
#include <math/Symbols.h>
#include <themes/NewComputerModern.h>
#include <visuals/Layout.h>
#include <visuals/MissingGlyph.h>
#include <visuals/Placement.h>

#include <fstream>
#include <regex>
#include <string>
#include <vector>

using Catch::Approx;
using namespace math;

// Every symbol in Symbols.h is read back from the file, so a new one is checked without being listed here as well
TEST_CASE( "every named math symbol is a glyph the math font has", "[math][symbols]" )
{
	const std::string path = std::string( SONIMATION_TEST_DATA ) + "/../../src/math/Symbols.h";
	std::ifstream file( path );
	REQUIRE( file );

	const std::regex definition( R"re(inline constexpr const char \*(\w+) = "([^"]+)";)re" );
	std::vector<std::pair<std::string, std::string>> symbols;
	std::string line;
	while ( std::getline( file, line ) )
	{
		std::smatch match;
		if ( std::regex_search( line, match, definition ) )
		{
			symbols.emplace_back( match[1], match[2] );
		}
	}
	CHECK( symbols.size() > 90 );

	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	clear_missing_glyph_reports();
	for ( const auto &[name, glyphs]: symbols )
	{
		const size_t before = missing_glyph_reports().size();
		CHECK( text( glyphs ).build( context ) != nullptr );
		INFO( "sym::" << name << " = " << glyphs );
		CHECK( missing_glyph_reports().size() == before );
	}
	clear_missing_glyph_reports();
}

TEST_CASE( "named symbols are the characters they say", "[math][symbols]" )
{
	CHECK( std::string( sym::alpha ) == "α" );
	CHECK( std::string( sym::leq ) == "≤" );
	CHECK( std::string( sym::infty ) == "∞" );
	CHECK( std::string( sym::Rightarrow ) == "⇒" );
}

TEST_CASE( "upright text with spaces keeps its spaces (words in a formula)", "[math][symbols]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	auto width = [&]( const Expr &e ) {
		const AABB box = layout_extent( *e.build( context ) );
		return box.max.x() - box.min.x();
	};
	CHECK( width( upright( "for all" ) ) > width( upright( "forall" ) ) + 0.05F );
}

TEST_CASE( "primed puts prime marks up after its base, one for each", "[math][symbols]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	auto width = [&]( const Expr &e ) {
		const AABB box = layout_extent( *e.build( context ) );
		return box.max.x() - box.min.x();
	};
	const float plain = width( text( "f" ) );
	const float one = width( primed( text( "f" ) ) );
	const float two = width( primed( text( "f" ), 2 ) );
	CHECK( one > plain );
	CHECK( two > one );
	CHECK( width( primed( text( "f" ), 0 ) ) == one ); // (at least one)
}

TEST_CASE( "words is upright text", "[math][symbols]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	const AABB a = layout_extent( *words( "if x" ).build( context ) );
	const AABB b = layout_extent( *upright( "if x" ).build( context ) );
	CHECK( a.max.x() == b.max.x() );
}

TEST_CASE( "cancel takes no room, boxed takes the padding, colored colors", "[math][marks]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	const AABB plain = layout_extent( *text( "xy" ).build( context ) );

	const AABB cancelled = layout_extent( *cancel( text( "xy" ) ).build( context ) );
	CHECK( cancelled.max.x() == Approx( plain.max.x() ) );
	CHECK( cancelled.min.y() == Approx( plain.min.y() ) );
	CHECK( cancelled.max.y() == Approx( plain.max.y() ) );

	const AABB framed = layout_extent( *boxed( text( "xy" ), 0.5F ).build( context ) );
	CHECK( framed.max.x() > plain.max.x() );
	CHECK( framed.min.y() < plain.min.y() );
	// The frame is the same amount past the content on every side
	CHECK( plain.min.x() - framed.min.x() == Approx( framed.max.x() - plain.max.x() ) );
	CHECK( plain.min.y() - framed.min.y() == Approx( framed.max.y() - plain.max.y() ) );

	auto red = colored( Color{ 1.F, 0.F, 0.F, 1.F }, text( "x" ) ).build( context );
	CHECK( red->color.r == 1.F );
	CHECK( red->color.g == 0.F );

	CHECK_FALSE( cancel( Expr() ) );
	CHECK_FALSE( boxed( Expr() ) );
	CHECK_FALSE( colored( Color{}, Expr() ) );
}

TEST_CASE( "aligned lines its cells up where the first and second meet", "[math][aligned]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	auto grid = aligned( { { text( "x" ), row( { rel( "=" ), ord( "a + b + c" ) } ) },
						   { text( "long name" ), row( { rel( "=" ), ord( "d" ) } ) } } )
					.build( context );
	const auto &group = dynamic_cast<const Group &>( *grid );
	std::vector<AABB> cells;
	for ( const auto &child: group )
	{
		cells.push_back( box_of( *child ) );
	}
	REQUIRE( cells.size() == 4 ); // two rows of a pair

	// The first cells end on one line, the second cells begin on one line
	CHECK( cells[0].max.x() == Approx( cells[2].max.x() ).margin( 0.01 ) );
	CHECK( cells[1].min.x() == Approx( cells[3].min.x() ).margin( 0.01 ) );
	// with the second right after the first
	CHECK( cells[1].min.x() == Approx( cells[0].max.x() ).margin( 0.01 ) );
	// and the second row is below
	CHECK( cells[2].max.y() < cells[0].max.y() );
}

TEST_CASE( "aligned numbers its lines, or uses the tags given", "[math][aligned]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	auto count = [&]( AlignedOptions options ) {
		auto grid = aligned( { { text( "a" ), text( "b" ) }, { text( "c" ), text( "d" ) } }, options ).build( context );
		return dynamic_cast<const Group &>( *grid ).size();
	};
	const size_t plain = count( {} );
	CHECK( count( { .numbered = true } ) == plain + 4 ); // a gap and a number in each row
	CHECK( count( { .tags = { "(*)", "" } } ) == plain + 4 );
}
