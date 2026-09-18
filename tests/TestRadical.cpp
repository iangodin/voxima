#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <fonts/MathFont.h>
#include <themes/AcademicClassic.h>
#include <visuals/Bounds.h>
#include <visuals/Circle.h>
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
