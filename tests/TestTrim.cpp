#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <visuals/Trim.h>

using Catch::Approx;

namespace
{
// A straight run from (0,0) to (4,0) as a cubic
Contour line()
{
	Contour c;
	c.start = { 0.F, 0.F };
	c.closed = false;
	c.segs.push_back( { { 1.F, 0.F }, { 3.F, 0.F }, { 4.F, 0.F } } );
	return c;
}
}

TEST_CASE( "trimmed_commands cuts a contour at a fraction of its length", "[trim]" )
{
	const Contour c = line();
	CHECK( contour_length( c ) == Approx( 4.F ) );

	auto half = trimmed_commands( c, 0.5F );
	REQUIRE( half.size() == 2 );
	CHECK( std::get<CubicTo>( half[1] ).to.x() == Approx( 2.F ) );

	auto none = trimmed_commands( c, 0.F );
	CHECK( none.size() == 1 );

	auto all = trimmed_commands( c, 1.F );
	REQUIRE( all.size() == 2 );
	CHECK( std::get<CubicTo>( all[1] ).to.x() == Approx( 4.F ) );
}

TEST_CASE( "a closed contour drawn in full is closed", "[trim]" )
{
	Contour c = line();
	c.closed = true;
	auto all = trimmed_commands( c, 1.F );
	CHECK( std::holds_alternative<Close>( all.back() ) );
	auto part = trimmed_commands( c, 0.9F );
	CHECK( !std::holds_alternative<Close>( part.back() ) );
}
