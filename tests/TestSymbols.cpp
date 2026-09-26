#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <math/Expr.h>
#include <math/Symbols.h>
#include <themes/NewComputerModern.h>
#include <visuals/Layout.h>
#include <visuals/MissingGlyph.h>

#include <fstream>
#include <regex>
#include <string>
#include <vector>

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
