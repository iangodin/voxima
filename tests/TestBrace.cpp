#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <themes/NewComputerModern.h>
#include <visuals/Brace.h>
#include <visuals/Rectangle.h>

#include <variant>
#include "TestThemes.h"

using Catch::Approx;

namespace
{
Point end_of( const Path &path )
{
	const PathCommand &last = path.commands.back();
	if ( const auto *cubic = std::get_if<CubicTo>( &last ) ) return cubic->to;
	return std::get<LineTo>( last ).to;
}

// The furthest a path's on-curve points get from the line y = 0, on the positive side
float highest( const Path &path )
{
	float top = 0.F;
	for ( const PathCommand &command: path.commands )
	{
		if ( const auto *cubic = std::get_if<CubicTo>( &command ) ) top = std::max( top, cubic->to.y() );
		if ( const auto *line = std::get_if<LineTo>( &command ) ) top = std::max( top, line->to.y() );
	}
	return top;
}
}

TEST_CASE( "a brace spans two points and stands out by its depth", "[brace]" )
{
	auto brace = make_brace( { 0.F, 0.F }, { 4.F, 0.F }, { .depth = 0.6F } );
	CHECK( std::get<MoveTo>( brace->commands.front() ).to.x() == Approx( 0.F ) );
	CHECK( end_of( *brace ).x() == Approx( 4.F ) );
	CHECK( end_of( *brace ).y() == Approx( 0.F ).margin( 1e-4 ) );
	// Along +x, to its left is up
	CHECK( highest( *brace ) == Approx( 0.6F ) );

	auto right = make_brace( { 0.F, 0.F }, { 4.F, 0.F }, { .depth = 0.6F, .left = false } );
	float lowest = 0.F;
	for ( const PathCommand &command: right->commands )
	{
		if ( const auto *cubic = std::get_if<CubicTo>( &command ) ) lowest = std::min( lowest, cubic->to.y() );
	}
	CHECK( lowest == Approx( -0.6F ) );
}

TEST_CASE( "a brace beside a visual stands out away from it", "[brace]" )
{
	Rectangle box;
	box.half_width = 2.F;
	box.half_height = 1.F;
	auto below = make_brace_beside( box, Anchor::BOTTOM, 0.2F, { .depth = 0.5F } );
	float lowest = 0.F;
	for ( const PathCommand &command: below->commands )
	{
		if ( const auto *cubic = std::get_if<CubicTo>( &command ) ) lowest = std::min( lowest, cubic->to.y() );
	}
	CHECK( lowest == Approx( -1.F - 0.2F - 0.5F ) );

	auto right = make_brace_beside( box, Anchor::RIGHT, 0.2F, { .depth = 0.5F } );
	float furthest = 0.F;
	for ( const PathCommand &command: right->commands )
	{
		if ( const auto *cubic = std::get_if<CubicTo>( &command ) ) furthest = std::max( furthest, cubic->to.x() );
	}
	CHECK( furthest == Approx( 2.F + 0.2F + 0.5F ) );
}

TEST_CASE( "a labeled brace puts its label beyond the brace's point", "[brace]" )
{
	Theme theme = test_theme();
	Rectangle box;
	box.half_width = 2.F;
	box.half_height = 1.F;
	auto braced = make_labeled_brace_beside( theme, box, Anchor::TOP, "width", FontStyle{ Intent::LABEL }, 0.2F, { .depth = 0.5F, .label_gap = 0.2F } );
	REQUIRE( braced->size() == 2 );
	const AABB label = box_of( **( braced->begin() + 1 ) );
	CHECK( label.min.y() == Approx( 1.F + 0.2F + 0.5F + 0.2F ).margin( 1e-3 ) );
	CHECK( ( label.min.x() + label.max.x() ) / 2.F == Approx( 0.F ).margin( 1e-3 ) );
}
