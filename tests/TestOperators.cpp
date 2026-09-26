#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <math/Operators.h>
#include <themes/NewComputerModern.h>
#include <visuals/Fraction.h>
#include <visuals/Layout.h>

using Catch::Approx;
using namespace math;
using namespace math::literals;

namespace
{
float width( const Context &context, const Expr &e )
{
	const AABB box = layout_extent( *e.build( context ) );
	return box.max.x() - box.min.x();
}
}

TEST_CASE( "operators build the rows written by hand", "[math][operators]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };

	CHECK( width( context, "a"_m + "b"_m ) == Approx( width( context, row( { ord( text( "a" ) ), bin( "+" ), ord( text( "b" ) ) } ) ) ) );
	CHECK( width( context, "a"_m - "b"_m ) == Approx( width( context, row( { ord( text( "a" ) ), bin( "−" ), ord( text( "b" ) ) } ) ) ) );
	CHECK( width( context, eq( "a"_m, "b"_m ) ) == Approx( width( context, row( { ord( text( "a" ) ), rel( "=" ), ord( text( "b" ) ) } ) ) ) );
	CHECK( width( context, "a"_m * "b"_m ) == Approx( width( context, row( { ord( text( "a" ) ), ord( text( "b" ) ) } ) ) ) );

	// A product has no space in it, a sum does
	CHECK( width( context, "a"_m * "b"_m ) < width( context, "a"_m + "b"_m ) );
	CHECK( width( context, times( "a"_m, "b"_m ) ) > width( context, "a"_m * "b"_m ) );
}

TEST_CASE( "operators nest as C++ precedence says", "[math][operators]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };

	// a b + c d: the products bind tighter
	const Expr both = "a"_m * "b"_m + "c"_m * "d"_m;
	const Expr by_hand = row( { ord( row( { ord( text( "a" ) ), ord( text( "b" ) ) } ) ), bin( "+" ), ord( row( { ord( text( "c" ) ), ord( text( "d" ) ) } ) ) } );
	CHECK( width( context, both ) == Approx( width( context, by_hand ) ) );

	// A slash is a fraction
	auto built = ( "1"_m / "2"_m ).build( context );
	CHECK( dynamic_cast<Fraction *>( built.get() ) != nullptr );

	// −a: the sign close up to what it signs, so narrower than a subtraction
	CHECK( width( context, -"a"_m ) < width( context, "b"_m - "a"_m ) );
	CHECK( width( context, -"a"_m ) == Approx( width( context, row( { ord( "−" ), ord( text( "a" ) ) } ) ) ) );
}

TEST_CASE( "relations by name", "[math][operators]" )
{
	Theme theme = NewComputerModern::create_theme();
	Context context{ theme };
	for ( auto make: { eq, lt, gt, leq, geq, neq } )
	{
		CHECK( width( context, make( "x"_m, "y"_m ) ) > width( context, "x"_m + "y"_m ) - 1.F );
	}
	CHECK( width( context, relation( "x"_m, sym::approx, "y"_m ) ) == Approx( width( context, row( { ord( text( "x" ) ), rel( sym::approx ), ord( text( "y" ) ) } ) ) ) );
	CHECK( width( context, "sin"_u ) == Approx( width( context, upright( "sin" ) ) ) );
}
