#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/Circle.h>
#include <visuals/Delimited.h>
#include <visuals/Fraction.h>
#include <visuals/Layout.h>

#include <algorithm>
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
	float em = math_fonts.roman().scale;
	float axis = font.axis_height() * s;

	static std::unique_ptr<Circle> circle( float radius )
	{
		auto c = std::make_unique<Circle>();
		c->radius = radius;
		return c;
	}
};
}

TEST_CASE( "Delimited: delimiters fit the content", "[visuals]" )
{
	Fixture f;
	auto build = [&]( float radius )
	{
		return std::make_unique<Delimited>( f.font, f.s, U'(', Fixture::circle( radius ), U')' );
	};

	auto delimited = build( 0.5F );
	REQUIRE( delimited->size() == 3 );
	auto it = delimited->begin();
	const Visual &open = **it++;
	const Visual &content = **it++;
	const Visual &close = **it;

	const AABB o = layout_extent( open );
	const AABB c = layout_extent( content );
	const AABB cl = layout_extent( close );

	SECTION( "they are at least as tall as TeX's rule asks for" )
	{
		const float delta = std::max( 0.5F - f.axis, 0.5F + f.axis );
		const float target = std::max( 0.901F * 2.F * delta, 2.F * delta - 0.5F * f.em );
		CHECK( o.height() >= 0.95F * target );
		CHECK( cl.height() >= 0.95F * target );
	}

	SECTION( "they are centered on the math axis" )
	{
		CHECK( ( o.min.y() + o.max.y() ) / 2.F == Approx( f.axis ).margin( 0.05 ) );
		CHECK( ( cl.min.y() + cl.max.y() ) / 2.F == Approx( f.axis ).margin( 0.05 ) );
	}

	SECTION( "open, content and close follow each other with no gap" )
	{
		CHECK( o.min.x() == Approx( 0.F ).margin( 1e-4 ) );
		CHECK( c.min.x() == Approx( o.max.x() ).margin( 1e-4 ) );
		CHECK( cl.min.x() == Approx( c.max.x() ).margin( 1e-4 ) );
		REQUIRE( delimited->extent.has_value() );
		CHECK( delimited->extent->max.x() == Approx( cl.max.x() ).margin( 1e-4 ) );
	}

	SECTION( "the content stays on the baseline" )
	{
		CHECK( content.position.y() + content.baseline == Approx( 0.F ) );
	}

	SECTION( "taller content gets taller delimiters" )
	{
		auto tall = build( 1.5F );
		const AABB tall_open = layout_extent( **tall->begin() );
		CHECK( tall_open.height() > o.height() );
	}
}

TEST_CASE( "Delimited: either delimiter is optional", "[visuals]" )
{
	Fixture f;

	Delimited none( f.font, f.s, 0, Fixture::circle( 0.5F ), 0 );
	CHECK( none.size() == 1 );

	Delimited left_only( f.font, f.s, U'[', Fixture::circle( 0.5F ), 0 );
	CHECK( left_only.size() == 2 );

	Delimited right_only( f.font, f.s, 0, Fixture::circle( 0.5F ), U']' );
	CHECK( right_only.size() == 2 );
}

TEST_CASE( "Fraction without a rule (stack)", "[visuals]" )
{
	Fixture f;
	const FractionOptions stack_options{ .rule = false, .side_space = false, .style = MathStyle::text() };
	const float gap_min = f.font.stack_gap_min() * f.s;
	const float shift_up = f.font.stack_top_shift_up() * f.s;
	const float shift_down = f.font.stack_bottom_shift_down() * f.s;

	SECTION( "small children sit at the stack's own shifts and there is no rule" )
	{
		Fraction stack( f.font, f.s, Fixture::circle( 0.01F ), Fixture::circle( 0.01F ), stack_options );
		REQUIRE( stack.size() == 2 );
		auto it = stack.begin();
		const Visual &top = **it++;
		const Visual &bottom = **it;

		CHECK( top.position.y() == Approx( shift_up ) );
		CHECK( bottom.position.y() == Approx( -shift_down ) );
	}

	SECTION( "large children are pushed apart to the minimum gap" )
	{
		Fraction stack( f.font, f.s, Fixture::circle( 0.6F ), Fixture::circle( 0.6F ), stack_options );
		auto it = stack.begin();
		const AABB top = layout_extent( **it++ );
		const AABB bottom = layout_extent( **it );

		CHECK( top.min.y() - bottom.max.y() == Approx( gap_min ) );
	}

	SECTION( "with no side space the box is as wide as the wider child" )
	{
		Fraction stack( f.font, f.s, Fixture::circle( 0.1F ), Fixture::circle( 0.3F ), stack_options );
		REQUIRE( stack.extent.has_value() );
		CHECK( stack.extent->min.x() == Approx( 0.F ) );
		CHECK( stack.extent->max.x() == Approx( 0.6F ) );
	}

	SECTION( "side space is still the default" )
	{
		Fraction stack( f.font, f.s, Fixture::circle( 0.1F ), Fixture::circle( 0.3F ), FractionOptions{ .rule = false } );
		CHECK( stack.extent->max.x() > 0.6F );
	}
}

TEST_CASE( "make_binomial", "[visuals]" )
{
	Fixture f;
	auto binomial = make_binomial( f.font, f.s, Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) );

	auto *delimited = dynamic_cast<Delimited *>( binomial.get() );
	REQUIRE( delimited != nullptr );
	REQUIRE( delimited->size() == 3 );

	auto it = delimited->begin();
	++it;
	const auto *stack = dynamic_cast<const Fraction *>( it->get() );
	REQUIRE( stack != nullptr );
	// numerator and denominator only: no rule
	CHECK( stack->size() == 2 );
}
