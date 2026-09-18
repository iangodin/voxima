#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <fonts/MathStyle.h>
#include <themes/AcademicClassic.h>
#include <visuals/CreateText.h>

using Catch::Approx;

TEST_CASE( "FontStyle size factor", "[fonts]" )
{
	FontStyle a{ Intent::MATH };
	FontStyle b = a;
	CHECK( a == b );

	b.scaled( 0.5F );
	CHECK( a != b );
	CHECK( ( a < b ) != ( b < a ) );

	FontStyle c = b;
	c.scaled( 0.5F );
	CHECK( c.properties().size_factor.value() == Approx( 0.25F ) );
}

TEST_CASE( "Theme honours the size factor", "[fonts]" )
{
	auto theme = AcademicClassic::create_theme();
	FontStyle body = FontStyle{ Intent::MATH }.large();

	const float base = theme.get_stack( body ).font( 0 ).scale;
	const float half = theme.get_stack( FontStyle( body ).scaled( 0.5F ) ).font( 0 ).scale;
	CHECK( half == Approx( base * 0.5F ) );
}

TEST_CASE( "script_style uses the font's MATH constants", "[fonts]" )
{
	auto theme = AcademicClassic::create_theme();
	FontStyle body = FontStyle{ Intent::MATH }.large();
	MathFonts math_fonts = theme.get_math( body );
	MathFont font( math_fonts.roman() );
	const float base = math_fonts.roman().scale;

	auto scale_of = [&]( ScriptLevel level )
	{
		return theme.get_math( script_style( body, font, level ) ).roman().scale;
	};

	CHECK( scale_of( ScriptLevel::TEXT ) == Approx( base ) );
	CHECK( scale_of( ScriptLevel::SCRIPT ) == Approx( base * font.script_percent_scale_down() / 100.F ) );
	CHECK(
		scale_of( ScriptLevel::SCRIPT_SCRIPT ) ==
		Approx( base * font.script_script_percent_scale_down() / 100.F ) );
	CHECK( scale_of( ScriptLevel::SCRIPT_SCRIPT ) < scale_of( ScriptLevel::SCRIPT ) );
	CHECK( scale_of( ScriptLevel::SCRIPT ) < scale_of( ScriptLevel::TEXT ) );

	SECTION( "the base style is not modified" )
	{
		script_style( body, font, ScriptLevel::SCRIPT_SCRIPT );
		CHECK( body.properties().size_factor.has_value() == false );
	}
}
