#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <fonts/MathFont.h>
#include <themes/AcademicClassic.h>
#include <visuals/Bounds.h>
#include <visuals/CreateText.h>
#include <visuals/Fraction.h>
#include <visuals/Layout.h>
#include <visuals/Rectangle.h>

TEST_CASE( "Fraction layout", "[visuals]" )
{
	auto theme = AcademicClassic::create_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont font( math_fonts.roman() );
	const float s =
		math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );

	Fraction fraction(
		font,
		s,
		make_text( theme, "12345", FontStyle{ Intent::MATH } ),
		make_text( theme, "x", FontStyle{ Intent::MATH } ) );

	REQUIRE( fraction.size() == 3 );
	auto it = fraction.begin();
	const Visual &num = **it++;
	const Visual &den = **it++;
	const auto &bar = static_cast<const Rectangle &>( **it );

	const AABB nb = layout_extent( num );
	const AABB db = layout_extent( den );

	using Catch::Approx;
	const float axis = font.axis_height() * s;
	const float rule = font.fraction_rule_thickness() * s;

	SECTION( "the rule is centered on the math axis with the font's thickness" )
	{
		CHECK( bar.position.y() == Approx( axis ) );
		CHECK( bar.half_height * 2.F == Approx( rule ) );
	}

	SECTION( "numerator sits above the rule and denominator below it" )
	{
		CHECK( nb.min.y() >= axis + rule / 2.F + font.fraction_numerator_gap_min() * s - 1e-4F );
		CHECK( db.max.y() <= axis - rule / 2.F - font.fraction_denominator_gap_min() * s + 1e-4F );
	}

	SECTION( "the rule spans the wider child and both are centered over it" )
	{
		CHECK( bar.half_width * 2.F == Approx( nb.width() ) );
		CHECK( ( db.min.x() + db.max.x() ) / 2.F == Approx( bar.position.x() ) );
		CHECK( ( nb.min.x() + nb.max.x() ) / 2.F == Approx( bar.position.x() ) );
	}

	SECTION( "the layout box adds the null-delimiter space on both sides" )
	{
		const float em = s * static_cast<float>( math_fonts.roman().face->units_per_EM );
		const float side = 0.12F * em;

		REQUIRE( fraction.extent.has_value() );
		CHECK( fraction.extent->min.x() == Approx( 0.F ) );
		CHECK( fraction.extent->max.x() == Approx( bar.half_width * 2.F + 2.F * side ) );
		// The space is invisible: the painted bar starts `side` inside the box
		CHECK( bar.position.x() - bar.half_width == Approx( side ) );
	}
}
