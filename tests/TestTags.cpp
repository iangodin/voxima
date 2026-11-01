#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <math/Expr.h>
#include <themes/NewComputerModern.h>
#include <visuals/Accent.h>
#include <visuals/Bars.h>
#include <visuals/Delimited.h>
#include <visuals/Fraction.h>
#include <visuals/Limits.h>
#include <visuals/Matrix.h>
#include <visuals/Radical.h>
#include <visuals/Rectangle.h>
#include <visuals/Scripts.h>
#include <visuals/Group.h>
#include <visuals/Row.h>
#include <visuals/Tags.h>
#include "TestThemes.h"

using namespace math;

namespace
{
struct Fixture
{
	Theme theme = test_theme();
	Context context{ theme };
};
}

TEST_CASE( "Tagged parts can be found after building", "[math]" )
{
	Fixture f;

	SECTION( "the numerator and denominator are the fraction's own children" )
	{
		auto built = fraction( tag( "num", text( "1" ) ), tag( "den", text( "2" ) ) ).build( f.context );
		auto *frac = dynamic_cast<Fraction *>( built.get() );
		REQUIRE( frac != nullptr );

		CHECK( find_tag( *built, "num" ) == frac->begin()->get() );
		CHECK( find_tag( *built, "den" ) == ( frac->begin() + 1 )->get() );
	}

	SECTION( "an unknown tag finds nothing" )
	{
		auto built = tag( "a", text( "x" ) ).build( f.context );
		CHECK( find_tag( *built, "b" ) == nullptr );
		CHECK( find_tagged( *built, "b" ).empty() );
	}

	SECTION( "the tagged part can be the whole formula, and the root counts" )
	{
		auto built = tag( "all", fraction( text( "1" ), text( "2" ) ) ).build( f.context );
		CHECK( find_tag( *built, "all" ) == built.get() );
	}

	SECTION( "a name used twice finds both, left to right" )
	{
		auto built = row( { ord( tag( "letter", text( "x" ) ) ), bin( "+" ), ord( tag( "letter", text( "y" ) ) ) } )
		                 .build( f.context );
		auto found = find_tagged( *built, "letter" );
		REQUIRE( found.size() == 2 );

		auto *r = static_cast<Row *>( built.get() );
		CHECK( found[0] == r->begin()->get() );
		CHECK( found[1] == ( r->begin() + 2 )->get() );
		CHECK( find_tag( *built, "letter" ) == found[0] );
	}

	SECTION( "a part can carry several names" )
	{
		auto built = tag( "outer", tag( "inner", text( "x" ) ) ).build( f.context );
		REQUIRE( find_tag( *built, "outer" ) != nullptr );
		CHECK( find_tag( *built, "outer" ) == find_tag( *built, "inner" ) );
	}

	SECTION( "tagging an empty part leaves it empty, so optional parts stay optional" )
	{
		CHECK_FALSE( tag( "none", Expr() ) );

		auto built = superscript( text( "x" ), tag( "sup", Expr() ) ).build( f.context );
		CHECK( find_tag( *built, "sup" ) == nullptr );
	}

	SECTION( "tags are found from any root, including the group holding the built formula" )
	{
		Group stage;
		stage.add( fraction( tag( "num", text( "1" ) ), text( "2" ) ).build( f.context ) );
		stage.add( sum( tag( "from", text( "n" ) ), tag( "to", text( "5" ) ) ).build( f.context ) );

		CHECK( find_tag( stage, "num" ) != nullptr );
		CHECK( find_tag( stage, "from" ) != nullptr );
		CHECK( find_tag( stage, "to" ) != nullptr );
		CHECK( find_tag( stage, "from" ) != find_tag( stage, "to" ) );
	}

	SECTION( "parts of composites and grids can be tagged" )
	{
		auto built = radical( fraction( text( "1" ), tag( "den", text( "2" ) ) ) ).build( f.context );
		CHECK( find_tag( *built, "den" ) != nullptr );

		auto grid = pmatrix( { { tag( "a", text( "a" ) ), text( "b" ) }, { text( "c" ), tag( "d", text( "d" ) ) } } )
		                .build( f.context );
		CHECK( find_tag( *grid, "a" ) != nullptr );
		CHECK( find_tag( *grid, "d" ) != nullptr );
		CHECK( find_tag( *grid, "a" ) != find_tag( *grid, "d" ) );
	}

	SECTION( "each build has its own tagged visuals" )
	{
		Expr formula = fraction( tag( "num", text( "1" ) ), text( "2" ) );
		auto first = formula.build( f.context );
		auto second = formula.build( f.context );
		REQUIRE( find_tag( *first, "num" ) != nullptr );
		REQUIRE( find_tag( *second, "num" ) != nullptr );
		CHECK( find_tag( *first, "num" ) != find_tag( *second, "num" ) );
	}

	SECTION( "the typed lookup" )
	{
		auto built = row( { ord( tag( "frac", fraction( text( "1" ), text( "2" ) ) ) ), ord( tag( "letter", text( "x" ) ) ) } )
		                 .build( f.context );
		CHECK( find_tag_as<Fraction>( *built, "frac" ) != nullptr );
		CHECK( find_tag_as<Fraction>( *built, "letter" ) == nullptr ); // found, but not a Fraction
		CHECK( find_tag_as<Fraction>( *built, "missing" ) == nullptr );
	}

	SECTION( "changing a tagged part changes only that part" )
	{
		auto built = fraction( tag( "num", text( "1" ) ), text( "2" ) ).build( f.context );
		auto *num = find_tag( *built, "num" );
		REQUIRE( num != nullptr );
		auto *frac = static_cast<Fraction *>( built.get() );
		const float den_y = ( frac->begin() + 1 )->get()->position.y();

		const float before = num->position.y();
		num->position = { num->position.x(), before + 1.F };
		num->color = Color{ 1.F, 0.F, 0.F, 1.F };

		CHECK( num->position.y() == before + 1.F );
		CHECK( ( frac->begin() + 1 )->get()->position.y() == den_y );
	}
}

TEST_CASE( "Tag paths and nearest-first lookup", "[math]" )
{
	Fixture f;

	SECTION( "the nearest match comes first: a fraction's own bar before ones nested in it" )
	{
		auto built = fraction( fraction( text( "1" ), text( "2" ) ), text( "3" ) ).build( f.context );
		auto bars = find_tagged( *built, "bar" );
		REQUIRE( bars.size() == 2 );

		auto *outer = static_cast<Fraction *>( built.get() );
		CHECK( bars[0] == ( outer->begin() + 2 )->get() ); // its children: numerator, denominator, bar
		CHECK( find_tag( *built, "bar" ) == bars[0] );
	}

	SECTION( "a path searches inside each match in turn" )
	{
		auto built = row( { ord( tag( "f1", fraction( text( "1" ), text( "2" ) ) ) ),
		                    ord( tag( "f2", fraction( text( "3" ), text( "4" ) ) ) ) } )
		                 .build( f.context );
		auto *r = static_cast<Row *>( built.get() );
		auto *f1 = static_cast<Fraction *>( r->begin()->get() );
		auto *f2 = static_cast<Fraction *>( ( r->begin() + 1 )->get() );

		CHECK( find_tagged( *built, "bar" ).size() == 2 );
		CHECK( find_tag( *built, "f1", "bar" ) == ( f1->begin() + 2 )->get() );
		CHECK( find_tag( *built, "f2", "bar" ) == ( f2->begin() + 2 )->get() );
		CHECK( find_tag( *built, "f2", "numerator" ) == f2->begin()->get() );
		CHECK( find_tag( *built, "f1", "bar" ) != find_tag( *built, "f2", "bar" ) );
	}

	SECTION( "a path can be as long as you like, and the typed lookup takes one too" )
	{
		auto built = tag( "outer", fraction( tag( "inner", fraction( text( "1" ), text( "2" ) ) ), text( "3" ) ) )
		                 .build( f.context );
		auto *inner_bar = find_tag( *built, "outer", "inner", "bar" );
		REQUIRE( inner_bar != nullptr );
		// the inner fraction's bar, not the outer one's
		CHECK( inner_bar != find_tag( *built, "bar" ) );
		CHECK( find_tag_as<Rectangle>( *built, "outer", "inner", "bar" ) == inner_bar );
		CHECK( find_tag_as<Fraction>( *built, "outer", "inner" ) != nullptr );
	}

	SECTION( "a step includes the visual it starts from" )
	{
		auto built = tag( "a", tag( "b", fraction( text( "1" ), text( "2" ) ) ) ).build( f.context );
		REQUIRE( find_tag( *built, "a", "b", "bar" ) != nullptr );
		CHECK( find_tag( *built, "a", "b", "bar" ) == find_tag( *built, "bar" ) );
	}

	SECTION( "a match found through more than one route is listed once" )
	{
		auto built = tag( "f", fraction( tag( "f", fraction( text( "1" ), text( "2" ) ) ), text( "3" ) ) )
		                 .build( f.context );
		auto bars = find_tagged( *built, "f", "bar" );
		REQUIRE( bars.size() == 2 );
		auto *outer = static_cast<Fraction *>( built.get() );
		CHECK( bars[0] == ( outer->begin() + 2 )->get() );
	}

	SECTION( "a step with no match finds nothing" )
	{
		auto built = tag( "f", fraction( text( "1" ), text( "2" ) ) ).build( f.context );
		CHECK( find_tag( *built, "f", "nothing" ) == nullptr );
		CHECK( find_tag( *built, "nothing", "bar" ) == nullptr );
		CHECK( find_tagged( *built, "f", "nothing" ).empty() );
	}
}

TEST_CASE( "Composites name their parts", "[math]" )
{
	Fixture f;

	SECTION( "fractions" )
	{
		auto built = fraction( text( "1" ), text( "2" ) ).build( f.context );
		auto *frac = static_cast<Fraction *>( built.get() );
		CHECK( find_tag( *built, "numerator" ) == frac->begin()->get() );
		CHECK( find_tag( *built, "denominator" ) == ( frac->begin() + 1 )->get() );
		CHECK( find_tag( *built, "bar" ) == ( frac->begin() + 2 )->get() );
	}

	SECTION( "a binomial has no bar" )
	{
		auto built = binomial( text( "n" ), text( "k" ) ).build( f.context );
		CHECK( find_tag( *built, "numerator" ) != nullptr );
		CHECK( find_tag( *built, "denominator" ) != nullptr );
		CHECK( find_tag( *built, "bar" ) == nullptr );
		CHECK( find_tag( *built, "open" ) != nullptr );
		CHECK( find_tag( *built, "close" ) != nullptr );
	}

	SECTION( "radicals" )
	{
		auto square = radical( text( "x" ) ).build( f.context );
		auto *rad = static_cast<Radical *>( square.get() );
		CHECK( find_tag( *square, "radicand" ) == rad->begin()->get() );
		CHECK( find_tag( *square, "overline" ) != nullptr );
		CHECK( find_tag( *square, "sign" ) != nullptr );
		CHECK( find_tag( *square, "index" ) == nullptr );

		auto cube = root( text( "3" ), text( "x" ) ).build( f.context );
		CHECK( find_tag( *cube, "index" ) != nullptr );
	}

	SECTION( "scripts" )
	{
		auto built = superscript( text( "x" ), text( "2" ) ).build( f.context );
		auto *sc = static_cast<Scripts *>( built.get() );
		CHECK( find_tag( *built, "base" ) == sc->begin()->get() );
		CHECK( find_tag( *built, "superscript" ) == ( sc->begin() + 1 )->get() );
		CHECK( find_tag( *built, "subscript" ) == nullptr );
	}

	SECTION( "large operators name their limits the same way in either form" )
	{
		auto over_under = sum( text( "n" ), text( "5" ) ).build( f.context );
		auto *limits = dynamic_cast<Limits *>( over_under.get() );
		REQUIRE( limits != nullptr );
		CHECK( find_tag( *over_under, "base" ) == limits->begin()->get() );
		CHECK( find_tag( *over_under, "upper" ) == ( limits->begin() + 1 )->get() );
		CHECK( find_tag( *over_under, "lower" ) == ( limits->begin() + 2 )->get() );

		auto at_side = sum( text( "n" ), text( "5" ) ).build( f.context, MathStyle::text() );
		auto *scripts_group = dynamic_cast<Scripts *>( at_side.get() );
		REQUIRE( scripts_group != nullptr );
		CHECK( find_tag( *at_side, "upper" ) == ( scripts_group->begin() + 1 )->get() );
		CHECK( find_tag( *at_side, "lower" ) == ( scripts_group->begin() + 2 )->get() );
		CHECK( find_tag( *at_side, "upper" ) == find_tag( *at_side, "superscript" ) );
	}

	SECTION( "delimiters, accents and rules" )
	{
		auto fenced_content = parens( text( "x" ) ).build( f.context );
		auto *d = static_cast<Delimited *>( fenced_content.get() );
		CHECK( find_tag( *fenced_content, "open" ) == d->begin()->get() );
		CHECK( find_tag( *fenced_content, "content" ) == ( d->begin() + 1 )->get() );
		CHECK( find_tag( *fenced_content, "close" ) == ( d->begin() + 2 )->get() );

		auto accented = hat( text( "x" ) ).build( f.context );
		CHECK( find_tag( *accented, "base" ) == static_cast<Accent *>( accented.get() )->begin()->get() );
		CHECK( find_tag( *accented, "accent" ) != nullptr );

		auto over = overline( text( "x" ) ).build( f.context );
		CHECK( find_tag( *over, "content" ) != nullptr );
		CHECK( find_tag_as<Rectangle>( *over, "line" ) != nullptr );
		auto under = underline( text( "x" ) ).build( f.context );
		CHECK( find_tag_as<Rectangle>( *under, "line" ) != nullptr );
	}

	SECTION( "matrix cells by position" )
	{
		auto built = matrix( { { text( "a" ), text( "b" ) }, { text( "c" ), text( "d" ) } } ).build( f.context );
		auto *grid = static_cast<Matrix *>( built.get() );
		CHECK( find_tagged( *built, "cell" ).size() == 4 );
		CHECK( find_tag( *built, "r0c0" ) == grid->begin()->get() );
		CHECK( find_tag( *built, "r0c1" ) == ( grid->begin() + 1 )->get() );
		CHECK( find_tag( *built, "r1c0" ) == ( grid->begin() + 2 )->get() );
		CHECK( find_tag( *built, "r1c1" ) == ( grid->begin() + 3 )->get() );
		CHECK( find_tag( *built, "r2c0" ) == nullptr );
	}

	SECTION( "the parts can be reached through a tag of your own" )
	{
		auto built = row( { ord( tag( "left", fraction( text( "1" ), text( "2" ) ) ) ),
		                    ord( tag( "right", radical( fraction( text( "3" ), text( "4" ) ) ) ) ) } )
		                 .build( f.context );
		CHECK( find_tag( *built, "left", "numerator" ) != nullptr );
		CHECK( find_tag( *built, "right", "sign" ) != nullptr );
		// the right-hand radical's fraction is found by path too
		CHECK( find_tag( *built, "right", "radicand", "bar" ) != nullptr );
		CHECK( find_tag( *built, "left", "sign" ) == nullptr );
	}
}
