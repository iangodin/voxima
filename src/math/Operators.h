#pragma once

#include "math/Expr.h"
#include "math/Symbols.h"

// Shorter ways to write formulas: the operators of C++ for the ones of math, with the spacing TeX gives them.
//
//     using namespace math;
//     using namespace math::literals;
//     auto sum_of_squares = eq( "a"_m * "a"_m + "b"_m * "b"_m, "c"_m * "c"_m );     // aa + bb = cc
//     auto half = "1"_m / "2"_m;                                                    // a fraction
//
// Each makes a row of atoms of the right classes (so the plus gets its space and the product none), and is an Expr like any other,
// to put in a bigger formula or to tag. They are found by argument-dependent lookup, so `a + b` on two Exprs needs no using.
namespace math
{

// a + b, a − b (a minus sign, not a hyphen), with the space round a binary operator
Expr operator+( Expr a, Expr b );
Expr operator-( Expr a, Expr b );

// −a: the sign close up to what it signs
Expr operator-( Expr a );

// a b: two things side by side with no sign between them (an implicit product). Use times( a, b ) for a × b.
Expr operator*( Expr a, Expr b );
Expr times( Expr a, Expr b );

// a over b, as a fraction
Expr operator/( Expr a, Expr b );

// Relations: a = b, a < b, a > b, a ≤ b, a ≥ b, a ≠ b (`==` and the other comparisons are not overloaded: they would say
// something else). Any other relation: relation( a, sym::approx, b ).
Expr relation( Expr a, const char *symbol, Expr b );
Expr eq( Expr a, Expr b );
Expr lt( Expr a, Expr b );
Expr gt( Expr a, Expr b );
Expr leq( Expr a, Expr b );
Expr geq( Expr a, Expr b );
Expr neq( Expr a, Expr b );

namespace literals
{

// "x"_m is text( "x" ) (math italic), "sin"_u is upright( "sin" )
Expr operator""_m( const char *characters, size_t size );
Expr operator""_u( const char *characters, size_t size );

}

}
