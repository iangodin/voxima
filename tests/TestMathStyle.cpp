#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <fonts/MathStyle.h>
#include <themes/AcademicClassic.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Fraction.h>
#include <visuals/Glyph.h>
#include <visuals/Layout.h>
#include <visuals/Radical.h>
#include <visuals/Scripts.h>
#include "TestThemes.h"

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
	auto theme = test_academic_theme();
	FontStyle body = FontStyle{ Intent::MATH }.large();

	const float base = theme.get_stack( body ).font( 0 ).scale;
	const float half = theme.get_stack( FontStyle( body ).scaled( 0.5F ) ).font( 0 ).scale;
	CHECK( half == Approx( base * 0.5F ) );
}

TEST_CASE( "script_style uses the font's MATH constants", "[fonts]" )
{
	auto theme = test_academic_theme();
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

TEST_CASE( "MathStyle: the style of each part of a formula", "[fonts]" )
{
	using L = MathStyle::Level;
	const MathStyle D = MathStyle::display();
	const MathStyle T = MathStyle::text();
	const MathStyle S = MathStyle::script();
	const MathStyle SS = MathStyle::script_script();

	SECTION( "numerators step down a size and keep the cramping" )
	{
		CHECK( D.numerator() == T );
		CHECK( T.numerator() == S );
		CHECK( S.numerator() == SS );
		CHECK( SS.numerator() == SS );
		CHECK( MathStyle( L::DISPLAY, true ).numerator() == MathStyle( L::TEXT, true ) );
	}

	SECTION( "denominators step down the same way and are cramped" )
	{
		CHECK( D.denominator() == MathStyle( L::TEXT, true ) );
		CHECK( T.denominator() == MathStyle( L::SCRIPT, true ) );
		CHECK( S.denominator() == MathStyle( L::SCRIPT_SCRIPT, true ) );
		CHECK( SS.denominator() == MathStyle( L::SCRIPT_SCRIPT, true ) );
	}

	SECTION( "superscripts are script size and keep the cramping; subscripts are always cramped" )
	{
		CHECK( D.superscript() == S );
		CHECK( T.superscript() == S );
		CHECK( S.superscript() == SS );
		CHECK( SS.superscript() == SS );
		CHECK( MathStyle( L::TEXT, true ).superscript() == MathStyle( L::SCRIPT, true ) );

		CHECK( D.subscript() == MathStyle( L::SCRIPT, true ) );
		CHECK( S.subscript() == MathStyle( L::SCRIPT_SCRIPT, true ) );
	}

	SECTION( "radicands are cramped at the same size; the index is scriptscript" )
	{
		CHECK( D.radicand() == MathStyle( L::DISPLAY, true ) );
		CHECK( S.radicand() == MathStyle( L::SCRIPT, true ) );
		CHECK( D.index() == SS );
		CHECK( MathStyle( L::DISPLAY, true ).index() == SS );
	}

	SECTION( "matrix cells are text style in a displayed matrix" )
	{
		CHECK( D.cell() == T );
		CHECK( T.cell() == T );
		CHECK( S.cell() == S );
	}

	SECTION( "sizes" )
	{
		CHECK( D.script_level() == ScriptLevel::TEXT );
		CHECK( T.script_level() == ScriptLevel::TEXT );
		CHECK( S.script_level() == ScriptLevel::SCRIPT );
		CHECK( SS.script_level() == ScriptLevel::SCRIPT_SCRIPT );
		CHECK( D.is_display() );
		CHECK_FALSE( T.is_display() );
		CHECK( MathStyle( ScriptLevel::SCRIPT ) == S );
		CHECK( MathStyle() == D );
	}
}

namespace
{
struct StyleFixture
{
	Theme theme = test_academic_theme();
	MathFonts math_fonts = theme.get_math( FontStyle{ Intent::MATH } );
	MathFont font{ math_fonts.roman() };
	float s = math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );

	static std::unique_ptr<Circle> circle( float radius )
	{
		auto c = std::make_unique<Circle>();
		c->radius = radius;
		return c;
	}

	static std::unique_ptr<Glyph> character()
	{
		auto glyph = std::make_unique<Glyph>();
		AABB box;
		box.add( { 0.F, 0.F } );
		box.add( { 0.3F, 0.2F } );
		glyph->extent = box;
		return glyph;
	}
};
}

TEST_CASE( "Display style uses the font's more open constants", "[visuals]" )
{
	StyleFixture f;

	SECTION( "a fraction's numerator and denominator sit at the display shifts" )
	{
		Fraction display( f.font, f.s, StyleFixture::circle( 0.01F ), StyleFixture::circle( 0.01F ),
			FractionOptions{ .style = MathStyle::display() } );
		Fraction text( f.font, f.s, StyleFixture::circle( 0.01F ), StyleFixture::circle( 0.01F ),
			FractionOptions{ .style = MathStyle::text() } );

		auto d = display.begin();
		auto t = text.begin();
		CHECK( ( *d )->position.y() == Approx( f.font.fraction_numerator_display_style_shift_up() * f.s ) );
		CHECK( ( *t )->position.y() == Approx( f.font.fraction_numerator_shift_up() * f.s ) );
		CHECK( ( *( d + 1 ) )->position.y() == Approx( -f.font.fraction_denominator_display_style_shift_down() * f.s ) );
		CHECK( ( *( t + 1 ) )->position.y() == Approx( -f.font.fraction_denominator_shift_down() * f.s ) );
		CHECK( ( *d )->position.y() >= ( *t )->position.y() );
	}

	SECTION( "a stack uses the display shifts too" )
	{
		FractionOptions options{ .rule = false, .side_space = false, .style = MathStyle::display() };
		Fraction stack( f.font, f.s, StyleFixture::circle( 0.01F ), StyleFixture::circle( 0.01F ), options );
		CHECK( ( *stack.begin() )->position.y() == Approx( f.font.stack_top_display_style_shift_up() * f.s ) );
	}

	SECTION( "a radical leaves more room above the radicand" )
	{
		Radical display( f.font, f.s, StyleFixture::circle( 0.3F ), MathStyle::display() );
		Radical text( f.font, f.s, StyleFixture::circle( 0.3F ), MathStyle::text() );
		CHECK( display.extent->max.y() >= text.extent->max.y() );
	}
}

TEST_CASE( "Cramped style lowers superscripts", "[visuals]" )
{
	StyleFixture f;
	auto build = [&]( MathStyle style )
	{
		return Scripts(
			f.font, f.s, StyleFixture::character(), StyleFixture::circle( 0.01F ), nullptr, style );
	};

	auto normal = build( MathStyle::display() );
	auto cramped = build( MathStyle::display().cramp() );
	const float normal_y = ( *( normal.begin() + 1 ) )->position.y();
	const float cramped_y = ( *( cramped.begin() + 1 ) )->position.y();

	CHECK( normal_y == Approx( f.font.superscript_shift() * f.s ) );
	CHECK( cramped_y == Approx( f.font.superscript_shift_cramped() * f.s ) );
	CHECK( cramped_y <= normal_y );
}
