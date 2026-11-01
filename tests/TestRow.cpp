#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/Circle.h>
#include <visuals/Layout.h>
#include <visuals/Row.h>
#include "TestThemes.h"

#include <ft2build.h>
#include FT_FREETYPE_H

using Catch::Approx;

namespace
{
struct Fixture
{
	Theme theme = test_academic_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont font{ math_fonts.roman() };
	float s = math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );
	float mu = s * static_cast<float>( math_fonts.roman().face->units_per_EM ) / 18.F;

	static std::unique_ptr<Visual> atom()
	{
		auto circle = std::make_unique<Circle>();
		circle->radius = 0.1F;
		return circle;
	}

	// The gaps between neighbouring atoms of a row built from `classes`
	std::vector<float> gaps( const std::vector<AtomClass> &classes, ScriptLevel level = ScriptLevel::TEXT )
	{
		Row row( font, s, level );
		for ( auto c: classes )
		{
			row.append( c, atom() );
		}
		std::vector<float> out;
		AABB prev;
		bool first = true;
		for ( const auto &child: row )
		{
			AABB box = layout_extent( *child );
			if ( !first )
			{
				out.push_back( box.min.x() - prev.max.x() );
			}
			prev = box;
			first = false;
		}
		return out;
	}
};
}

TEST_CASE( "Row: spacing between atoms", "[visuals]" )
{
	Fixture f;
	using A = AtomClass;

	SECTION( "ordinary atoms touch" )
	{
		auto g = f.gaps( { A::ORD, A::ORD } );
		REQUIRE( g.size() == 1 );
		CHECK( g[0] == Approx( 0.F ).margin( 1e-5 ) );
	}

	SECTION( "a binary operator gets medium space on both sides" )
	{
		auto g = f.gaps( { A::ORD, A::BIN, A::ORD } );
		REQUIRE( g.size() == 2 );
		CHECK( g[0] == Approx( 4.F * f.mu ) );
		CHECK( g[1] == Approx( 4.F * f.mu ) );
	}

	SECTION( "a relation gets thick space on both sides" )
	{
		auto g = f.gaps( { A::ORD, A::REL, A::ORD } );
		CHECK( g[0] == Approx( 5.F * f.mu ) );
		CHECK( g[1] == Approx( 5.F * f.mu ) );
	}

	SECTION( "punctuation is followed by a thin space" )
	{
		auto g = f.gaps( { A::ORD, A::PUNCT, A::ORD } );
		CHECK( g[0] == Approx( 0.F ).margin( 1e-5 ) );
		CHECK( g[1] == Approx( 3.F * f.mu ) );
	}

	SECTION( "delimiters hug their contents" )
	{
		auto g = f.gaps( { A::OPEN, A::ORD, A::CLOSE } );
		CHECK( g[0] == Approx( 0.F ).margin( 1e-5 ) );
		CHECK( g[1] == Approx( 0.F ).margin( 1e-5 ) );
	}

	SECTION( "an operator and an ordinary atom are separated by a thin space" )
	{
		auto g = f.gaps( { A::OP, A::ORD } );
		CHECK( g[0] == Approx( 3.F * f.mu ) );
	}

	SECTION( "a fraction (inner) gets thin space next to ordinary atoms" )
	{
		auto g = f.gaps( { A::ORD, A::INNER, A::ORD } );
		CHECK( g[0] == Approx( 3.F * f.mu ) );
		CHECK( g[1] == Approx( 3.F * f.mu ) );
	}
}

TEST_CASE( "Row: binary operators without operands become ordinary", "[visuals]" )
{
	Fixture f;
	using A = AtomClass;

	SECTION( "at the start, as in -x" )
	{
		auto g = f.gaps( { A::BIN, A::ORD } );
		CHECK( g[0] == Approx( 0.F ).margin( 1e-5 ) );
	}

	SECTION( "at the end" )
	{
		auto g = f.gaps( { A::ORD, A::BIN } );
		CHECK( g[0] == Approx( 0.F ).margin( 1e-5 ) );
	}

	SECTION( "before a relation, as in a - = b" )
	{
		auto g = f.gaps( { A::ORD, A::BIN, A::REL, A::ORD } );
		CHECK( g[0] == Approx( 0.F ).margin( 1e-5 ) );
		CHECK( g[1] == Approx( 5.F * f.mu ) );
		CHECK( g[2] == Approx( 5.F * f.mu ) );
	}

	SECTION( "after another binary operator, as in a + -b" )
	{
		auto g = f.gaps( { A::ORD, A::BIN, A::BIN, A::ORD } );
		// the second operator is a sign: a + (-b)
		CHECK( g[0] == Approx( 4.F * f.mu ) );
		CHECK( g[1] == Approx( 4.F * f.mu ) );
		CHECK( g[2] == Approx( 0.F ).margin( 1e-5 ) );
	}
}

TEST_CASE( "Row: script styles drop the medium and thick spaces", "[visuals]" )
{
	Fixture f;
	using A = AtomClass;

	for ( auto level: { ScriptLevel::SCRIPT, ScriptLevel::SCRIPT_SCRIPT } )
	{
		auto bin = f.gaps( { A::ORD, A::BIN, A::ORD }, level );
		CHECK( bin[0] == Approx( 0.F ).margin( 1e-5 ) );
		auto rel = f.gaps( { A::ORD, A::REL, A::ORD }, level );
		CHECK( rel[0] == Approx( 0.F ).margin( 1e-5 ) );
		// unparenthesized entries stay: operator next to an ordinary atom
		auto op = f.gaps( { A::OP, A::ORD }, level );
		CHECK( op[0] == Approx( 3.F * f.mu ) );
	}
}

TEST_CASE( "Row: layout", "[visuals]" )
{
	Fixture f;
	using A = AtomClass;

	SECTION( "children are aligned on their baselines" )
	{
		Row row( f.font, f.s );
		auto high = Fixture::atom();
		high->position = { 0.F, 1.F };
		auto low = Fixture::atom();
		low->position = { 0.F, -2.F };
		low->baseline = -0.5F; // its baseline is half a unit below its centre
		auto &a = row.append( A::ORD, std::move( high ) );
		auto &b = row.append( A::ORD, std::move( low ) );
		CHECK( a.position.y() + a.baseline == Approx( 0.F ) );
		CHECK( b.position.y() + b.baseline == Approx( 0.F ) );
	}

	SECTION( "the box starts at the origin and ends after the last atom" )
	{
		Row row( f.font, f.s );
		row.append( A::ORD, Fixture::atom() );
		row.append( A::REL, Fixture::atom() );
		row.append( A::ORD, Fixture::atom() );

		REQUIRE( row.extent.has_value() );
		CHECK( row.extent->min.x() == Approx( 0.F ) );
		CHECK( row.extent->max.x() == Approx( 0.6F + 2.F * 5.F * f.mu ) );
		CHECK( row.extent->height() == Approx( 0.2F ) );
	}

	SECTION( "appending lays out again, so one at a time equals all at once" )
	{
		auto one_by_one = f.gaps( { A::ORD, A::BIN, A::ORD, A::REL, A::ORD } );
		// the same list built in one go is what gaps() does; compare with the expected sums
		REQUIRE( one_by_one.size() == 4 );
		CHECK( one_by_one[0] == Approx( 4.F * f.mu ) );
		CHECK( one_by_one[1] == Approx( 4.F * f.mu ) );
		CHECK( one_by_one[2] == Approx( 5.F * f.mu ) );
		CHECK( one_by_one[3] == Approx( 5.F * f.mu ) );
	}

	SECTION( "the row scales as a unit" )
	{
		Row row( f.font, f.s );
		row.append( A::ORD, Fixture::atom() );
		row.append( A::REL, Fixture::atom() );
		const float width = row.extent->max.x();
		row *= 2.F;
		CHECK( row.extent->max.x() == Approx( width * 2.F ) );
	}

	SECTION( "an empty row has no extent" )
	{
		Row row( f.font, f.s );
		CHECK_FALSE( row.extent.has_value() );
	}
}

TEST_CASE( "Row::spacing_mu table", "[visuals]" )
{
	using A = AtomClass;
	CHECK( Row::spacing_mu( A::ORD, A::BIN, ScriptLevel::TEXT ) == 4 );
	CHECK( Row::spacing_mu( A::BIN, A::ORD, ScriptLevel::TEXT ) == 4 );
	CHECK( Row::spacing_mu( A::REL, A::ORD, ScriptLevel::TEXT ) == 5 );
	CHECK( Row::spacing_mu( A::CLOSE, A::OP, ScriptLevel::TEXT ) == 3 );
	CHECK( Row::spacing_mu( A::PUNCT, A::PUNCT, ScriptLevel::TEXT ) == 3 );
	CHECK( Row::spacing_mu( A::PUNCT, A::PUNCT, ScriptLevel::SCRIPT ) == 0 );
	CHECK( Row::spacing_mu( A::OPEN, A::INNER, ScriptLevel::TEXT ) == 0 );
	CHECK( Row::spacing_mu( A::INNER, A::CLOSE, ScriptLevel::TEXT ) == 0 );
}

TEST_CASE( "Row: explicit space", "[visuals]" )
{
	Fixture f;
	using A = AtomClass;

	auto gap = []( Row &row )
	{
		auto it = row.begin();
		const AABB first = layout_extent( **it++ );
		const AABB second = layout_extent( **it );
		return second.min.x() - first.max.x();
	};

	SECTION( "a thin space between ordinary atoms, as in x\\,dx" )
	{
		Row row( f.font, f.s );
		row.append( A::ORD, Fixture::atom() );
		row.append_space( Row::THIN );
		row.append( A::ORD, Fixture::atom() );
		CHECK( gap( row ) == Approx( 3.F * f.mu ) );
	}

	SECTION( "it adds to the automatic spacing" )
	{
		Row row( f.font, f.s );
		row.append( A::ORD, Fixture::atom() );
		row.append_space( Row::THIN );
		row.append( A::REL, Fixture::atom() );
		CHECK( gap( row ) == Approx( ( 5.F + 3.F ) * f.mu ) );
	}

	SECTION( "spaces accumulate and can be negative" )
	{
		Row row( f.font, f.s );
		row.append( A::ORD, Fixture::atom() );
		row.append_space( Row::THICK );
		row.append_space( -Row::THIN );
		row.append( A::ORD, Fixture::atom() );
		CHECK( gap( row ) == Approx( 2.F * f.mu ) );
	}

	SECTION( "it is glue: it doesn't stop a binary operator turning ordinary" )
	{
		Row row( f.font, f.s );
		row.append_space( Row::THIN );
		row.append( A::BIN, Fixture::atom() );
		row.append( A::ORD, Fixture::atom() );
		auto it = row.begin();
		const AABB bin = layout_extent( **it++ );
		const AABB ord = layout_extent( **it );
		// the operator has no left operand, so it is ordinary: nothing between the two atoms
		CHECK( ord.min.x() - bin.max.x() == Approx( 0.F ).margin( 1e-5 ) );
	}

	SECTION( "space at the end widens the row" )
	{
		Row row( f.font, f.s );
		row.append( A::ORD, Fixture::atom() );
		row.append_space( Row::THIN );
		CHECK( row.extent->max.x() == Approx( 0.2F + 3.F * f.mu ) );
	}
}
