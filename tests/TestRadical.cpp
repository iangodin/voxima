#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <fonts/MathFont.h>
#include <themes/AcademicClassic.h>
#include <visuals/Bounds.h>
#include <visuals/Circle.h>
#include <visuals/Layout.h>
#include <visuals/Radical.h>

TEST_CASE( "Radical scaling scales the whole radical", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont math_font( math_fonts.roman() );
	const float glyph_scale =
		math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );

	Radical radical( math_font, glyph_scale, std::make_unique<Circle>() );

	BoundingBoxVisitor before;
	radical.accept( before );

	radical *= 2.F;

	BoundingBoxVisitor after;
	radical.accept( after );

	using Catch::Approx;
	CHECK( after.bounds.min.x() == Approx( before.bounds.min.x() * 2.F ).margin( 1e-4 ) );
	CHECK( after.bounds.max.x() == Approx( before.bounds.max.x() * 2.F ).margin( 1e-4 ) );
	CHECK( after.bounds.min.y() == Approx( before.bounds.min.y() * 2.F ).margin( 1e-4 ) );
	CHECK( after.bounds.max.y() == Approx( before.bounds.max.y() * 2.F ).margin( 1e-4 ) );
}

TEST_CASE( "Radical layout box", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont math_font( math_fonts.roman() );
	const float glyph_scale =
		math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );

	Radical radical( math_font, glyph_scale, std::make_unique<Circle>() );
	REQUIRE( radical.extent.has_value() );

	using Catch::Approx;
	// The box spans the sign's advance plus the radicand (a radius-1 circle, width 2)
	BoundingBoxVisitor ink;
	radical.accept( ink );
	CHECK( radical.extent->max.x() == Approx( ink.bounds.max.x() ) );
	CHECK( radical.extent->min.x() <= ink.bounds.min.x() + 1e-4F );
}

TEST_CASE( "Radical with an index", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont math_font( math_fonts.roman() );
	const float s =
		math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );

	auto index = std::make_unique<Circle>();
	index->radius = 0.05F;
	Radical radical( math_font, s, std::move( index ), std::make_unique<Circle>() );

	// children: radicand, overline, sign, index
	REQUIRE( radical.size() == 4 );
	auto it = radical.begin();
	const AABB content = layout_extent( **it++ );
	++it; // overline
	const AABB sign = layout_extent( **it++ );
	const AABB idx = layout_extent( **it );

	using Catch::Approx;
	const float kern_before = math_font.radical_kern_before_degree() * s;
	const float kern_after = math_font.radical_kern_after_degree() * s;
	const float percent = math_font.radical_degree_bottom_raise_percent();

	SECTION( "the index starts after the leading kern" )
	{
		CHECK( idx.min.x() == Approx( kern_before ) );
	}

	SECTION( "the sign is placed after kern, index, kern (no clamping, as in TeX)" )
	{
		CHECK( sign.min.x() == Approx( kern_before + idx.width() + kern_after ) );
	}

	SECTION( "the index bottom is raised a fixed percentage up the sign" )
	{
		CHECK( idx.min.y() == Approx( sign.min.y() + percent / 100.F * sign.height() ) );
	}

	SECTION( "the radicand follows the sign and the box starts at the origin" )
	{
		CHECK( content.min.x() == Approx( sign.max.x() ) );
		REQUIRE( radical.extent.has_value() );
		CHECK( radical.extent->min.x() == Approx( 0.F ) );
	}
}

TEST_CASE( "Radical index of any width", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont math_font( math_fonts.roman() );
	const float s =
		math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );
	const float kern_before = math_font.radical_kern_before_degree() * s;
	const float kern_after = math_font.radical_kern_after_degree() * s;

	auto build = [&]( float index_radius )
	{
		auto index = std::make_unique<Circle>();
		index->radius = index_radius;
		return std::make_unique<Radical>( math_font, s, std::move( index ), std::make_unique<Circle>() );
	};

	using Catch::Approx;

	SECTION( "a wide index pushes the sign right and overlaps it only by the kern" )
	{
		auto radical = build( 2.F );
		auto it = radical->begin();
		++it; ++it;
		const AABB sign = layout_extent( **it++ );
		const AABB idx = layout_extent( **it );

		CHECK( sign.min.x() == Approx( kern_before + idx.width() + kern_after ) );
		CHECK( idx.max.x() - sign.min.x() == Approx( -kern_after ) );
		CHECK( radical->extent->min.x() == Approx( 0.F ) );
		CHECK( radical->extent->max.x() > sign.max.x() );
	}

	SECTION( "a very narrow index leaves the sign at the summed position, not clamped" )
	{
		auto radical = build( 1e-4F );
		auto it = radical->begin();
		++it; ++it;
		const AABB sign = layout_extent( **it++ );
		const AABB idx = layout_extent( **it );

		CHECK( sign.min.x() == Approx( kern_before + idx.width() + kern_after ) );
		CHECK( radical->extent->min.x() == Approx( 0.F ) );
	}
}
