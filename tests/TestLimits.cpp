#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/Circle.h>
#include <visuals/CreateMathLargeOperation.h>
#include <visuals/LargeOperator.h>
#include <visuals/Layout.h>
#include <visuals/Limits.h>
#include <visuals/MathGlyph.h>
#include <visuals/Scripts.h>

using Catch::Approx;

namespace
{
struct Fixture
{
	Theme theme = AcademicClassic::create_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont font{ math_fonts.roman() };
	float s = math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );

	static std::unique_ptr<Circle> circle( float radius )
	{
		auto c = std::make_unique<Circle>();
		c->radius = radius;
		return c;
	}

	// A stand-in for a slanted operator: a box with an italic correction
	static std::unique_ptr<MathGlyph> slanted( float italic )
	{
		auto glyph = std::make_unique<MathGlyph>();
		AABB box;
		box.add( { 0.F, -0.5F } );
		box.add( { 1.F, 0.5F } );
		glyph->extent = box;
		glyph->italicCorrection = italic;
		return glyph;
	}
};

float center_x( const AABB &box )
{
	return ( box.min.x() + box.max.x() ) / 2.F;
}
}

TEST_CASE( "Limits: vertical placement", "[visuals]" )
{
	Fixture f;
	Limits limits( f.font, f.s, Fixture::circle( 0.5F ), Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) );
	REQUIRE( limits.size() == 3 );

	auto it = limits.begin();
	const Visual &nucleus = **it++;
	const Visual &upper = **it++;
	const Visual &lower = **it;

	const AABB n = layout_extent( nucleus );
	const AABB up = layout_extent( upper );
	const AABB low = layout_extent( lower );

	CHECK( nucleus.position.y() + nucleus.baseline == Approx( 0.F ) );

	SECTION( "the upper limit clears the nucleus by the font's minimums" )
	{
		CHECK( up.min.y() >= n.max.y() + f.font.upper_limit_gap_min() * f.s - 1e-4F );
		CHECK( upper.position.y() >= n.max.y() + f.font.upper_limit_baseline_rise_min() * f.s - 1e-4F );
	}

	SECTION( "the lower limit clears the nucleus by the font's minimums" )
	{
		CHECK( low.max.y() <= n.min.y() - f.font.lower_limit_gap_min() * f.s + 1e-4F );
		CHECK( lower.position.y() <= n.min.y() - f.font.lower_limit_baseline_drop_min() * f.s + 1e-4F );
	}
}

TEST_CASE( "Limits: horizontal placement", "[visuals]" )
{
	Fixture f;

	SECTION( "everything is centered on the widest part" )
	{
		Limits limits( f.font, f.s, Fixture::circle( 0.5F ), Fixture::circle( 0.2F ), Fixture::circle( 0.1F ) );
		auto it = limits.begin();
		const AABB n = layout_extent( **it++ );
		const AABB up = layout_extent( **it++ );
		const AABB low = layout_extent( **it );

		CHECK( center_x( n ) == Approx( 0.5F ) );
		CHECK( center_x( up ) == Approx( 0.5F ) );
		CHECK( center_x( low ) == Approx( 0.5F ) );
		REQUIRE( limits.extent.has_value() );
		CHECK( limits.extent->min.x() == Approx( 0.F ) );
		CHECK( limits.extent->max.x() == Approx( 1.F ) );
	}

	SECTION( "a wide limit widens the box and the nucleus stays centered" )
	{
		Limits limits( f.font, f.s, Fixture::circle( 0.5F ), Fixture::circle( 1.F ), nullptr );
		auto it = limits.begin();
		const AABB n = layout_extent( **it++ );
		const AABB up = layout_extent( **it );

		CHECK( limits.extent->max.x() == Approx( 2.F ) );
		CHECK( center_x( n ) == Approx( 1.F ) );
		CHECK( center_x( up ) == Approx( 1.F ) );
	}

	SECTION( "a slanted operator pushes the upper limit right and the lower limit left" )
	{
		Limits limits( f.font, f.s, Fixture::slanted( 0.2F ), Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) );
		auto it = limits.begin();
		++it;
		const AABB up = layout_extent( **it++ );
		const AABB low = layout_extent( **it );

		CHECK( center_x( up ) == Approx( 0.5F + 0.1F ) );
		CHECK( center_x( low ) == Approx( 0.5F - 0.1F ) );
	}
}

TEST_CASE( "Limits: either limit is optional", "[visuals]" )
{
	Fixture f;

	Limits only_upper( f.font, f.s, Fixture::circle( 0.5F ), Fixture::circle( 0.1F ), nullptr );
	CHECK( only_upper.size() == 2 );

	Limits only_lower( f.font, f.s, Fixture::circle( 0.5F ), nullptr, Fixture::circle( 0.1F ) );
	CHECK( only_lower.size() == 2 );

	Limits bare( f.font, f.s, Fixture::circle( 0.5F ), nullptr, nullptr );
	CHECK( bare.size() == 1 );
	CHECK( bare.extent->max.x() == Approx( 1.F ) );
}

TEST_CASE( "Limits scale as a unit", "[visuals]" )
{
	Fixture f;
	Limits limits( f.font, f.s, Fixture::circle( 0.5F ), Fixture::circle( 0.1F ), Fixture::circle( 0.1F ) );
	const AABB before = *limits.extent;
	limits *= 2.F;
	CHECK( limits.extent->max.x() == Approx( before.max.x() * 2.F ) );
	CHECK( limits.extent->max.y() == Approx( before.max.y() * 2.F ) );
	CHECK( limits.extent->min.y() == Approx( before.min.y() * 2.F ) );
}

TEST_CASE( "Large operators", "[visuals]" )
{
	Fixture f;
	const char32_t SUM = 0x2211;
	const char32_t INTEGRAL = 0x222B;

	auto build = [&]( char32_t op, bool display, std::optional<LimitPlacement> placement = std::nullopt )
	{
		return make_large_operator(
			f.font,
			f.s,
			op,
			Fixture::circle( 0.05F ),
			Fixture::circle( 0.05F ),
			display ? MathStyle::display() : MathStyle::text(),
			placement );
	};

	SECTION( "default placement follows the operator" )
	{
		CHECK( default_limit_placement( SUM ) == LimitPlacement::OVER_UNDER );
		CHECK( default_limit_placement( 0x220F ) == LimitPlacement::OVER_UNDER );
		CHECK( default_limit_placement( INTEGRAL ) == LimitPlacement::SIDE );
		CHECK( default_limit_placement( 0x222E ) == LimitPlacement::SIDE );
		CHECK( is_integral( 0x222C ) );
		CHECK_FALSE( is_integral( SUM ) );
	}

	SECTION( "a sum in display style has its limits over and under" )
	{
		CHECK( dynamic_cast<Limits *>( build( SUM, true ).get() ) != nullptr );
	}

	SECTION( "a sum in text style has its limits at the side" )
	{
		CHECK( dynamic_cast<Scripts *>( build( SUM, false ).get() ) != nullptr );
	}

	SECTION( "an integral has its limits at the side, even in display style" )
	{
		CHECK( dynamic_cast<Scripts *>( build( INTEGRAL, true ).get() ) != nullptr );
	}

	SECTION( "the placement can be overridden in display style" )
	{
		CHECK( dynamic_cast<Limits *>( build( INTEGRAL, true, LimitPlacement::OVER_UNDER ).get() ) != nullptr );
		CHECK( dynamic_cast<Scripts *>( build( SUM, true, LimitPlacement::SIDE ).get() ) != nullptr );
	}

	SECTION( "the display operator is at least as tall as the font's minimum and taller than text size" )
	{
		auto display = build( SUM, true );
		auto text = build( SUM, false );
		const float display_height = layout_extent( **display->begin() ).height();
		const float text_height = layout_extent( **text->begin() ).height();

		CHECK( display_height >= 0.8F * f.font.display_operator_min_height() * f.s );
		CHECK( display_height >= text_height );
	}

	SECTION( "an operator glyph keeps its shift onto the math axis under baseline alignment" )
	{
		auto glyph = create_math_large_op( f.font, SUM, 0.F );
		CHECK( glyph->position.y() + glyph->baseline == Approx( 0.F ).margin( 1e-4 ) );
	}
}
