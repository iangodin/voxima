#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <fonts/MathStyle.h>
#include <themes/AcademicClassic.h>
#include <visuals/CreateText.h>
#include <visuals/Layout.h>
#include <visuals/Scripts.h>
#include <themes/NewComputerModern.h>
#include "TestThemes.h"

using Catch::Approx;

namespace
{
struct Fixture
{
	Theme theme = test_academic_theme();
	FontStyle body = FontStyle{ Intent::MATH }.large();
	MathFonts math_fonts = theme.get_math( body );
	MathFont font{ math_fonts.roman() };
	float s = math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );

	std::unique_ptr<Visual> text( const std::string &str, ScriptLevel level = ScriptLevel::TEXT )
	{
		return make_text( theme, str, script_style( body, font, level ) );
	}

	std::unique_ptr<Scripts> scripts( bool sup, bool sub )
	{
		return std::make_unique<Scripts>(
			font,
			s,
			text( "x" ),
			sup ? text( "2", ScriptLevel::SCRIPT ) : nullptr,
			sub ? text( "i", ScriptLevel::SCRIPT ) : nullptr );
	}
};

// The rightmost glyph of a piece of text
const Glyph *last_glyph( const Visual &visual )
{
	if ( const auto *glyph = dynamic_cast<const Glyph *>( &visual ) )
	{
		return glyph;
	}
	if ( const auto *group = dynamic_cast<const Group *>( &visual ) )
	{
		if ( group->size() > 0 )
		{
			return last_glyph( **( group->end() - 1 ) );
		}
	}
	return nullptr;
}

// The baseline of a placed child, in the Scripts' frame
float baseline_of( const Visual &visual )
{
	return visual.position.y() + visual.baseline;
}
}

TEST_CASE( "Scripts: superscript only", "[visuals]" )
{
	Fixture f;
	auto scripts = f.scripts( true, false );
	REQUIRE( scripts->size() == 2 );

	auto it = scripts->begin();
	const Visual &nucleus = **it++;
	const Visual &sup = **it;
	const AABB n = layout_extent( nucleus );
	const AABB sb = layout_extent( sup );

	CHECK( baseline_of( nucleus ) == Approx( 0.F ) );
	CHECK( n.min.x() == Approx( 0.F ) );

	SECTION( "raised by at least the font's shift and clear of the baseline" )
	{
		CHECK( baseline_of( sup ) >= f.font.superscript_shift() * f.s - 1e-4F );
		CHECK( sb.min.y() >= f.font.superscript_bottom_min() * f.s - 1e-4F );
	}

	SECTION( "sits to the right of the nucleus, and the box adds the script spacing" )
	{
		CHECK( sb.min.x() >= n.max.x() - 1e-4F );
		REQUIRE( scripts->extent.has_value() );
		CHECK( scripts->extent->min.x() == Approx( 0.F ) );
		CHECK( scripts->extent->max.x() == Approx( sb.max.x() + f.font.space_after_script() * f.s ) );
	}
}

TEST_CASE( "Scripts: subscript only", "[visuals]" )
{
	Fixture f;
	auto scripts = f.scripts( false, true );
	REQUIRE( scripts->size() == 2 );

	auto it = scripts->begin();
	++it;
	const Visual &sub = **it;
	const AABB sb = layout_extent( sub );

	CHECK( baseline_of( sub ) <= -f.font.subscript_shift() * f.s + 1e-4F );
	CHECK( sb.max.y() <= f.font.subscript_top_max() * f.s + 1e-4F );
}

TEST_CASE( "Scripts: both scripts", "[visuals]" )
{
	Fixture f;
	auto scripts = f.scripts( true, true );
	REQUIRE( scripts->size() == 3 );

	auto it = scripts->begin();
	++it;
	const AABB sup = layout_extent( **it++ );
	const AABB sub = layout_extent( **it );

	SECTION( "the scripts stay apart by the font's minimum gap" )
	{
		CHECK( sup.min.y() - sub.max.y() >= f.font.sub_superscript_gap_min() * f.s - 1e-4F );
	}

	SECTION( "the superscript is not too low" )
	{
		CHECK( sup.min.y() >= f.font.superscript_bottom_max_with_subscript() * f.s - 1e-4F );
	}

	SECTION( "the superscript is shifted right of the subscript by the nucleus's italic correction" )
	{
		CHECK( sup.min.x() - sub.min.x() == Approx( last_glyph( *scripts->begin()->get() )->italicCorrection ) );
	}
}

TEST_CASE( "Scripts: a box nucleus is lowered/raised by the baseline drops", "[visuals]" )
{
	Fixture f;
	// A Scripts nucleus is itself a box (not a character run), so the drop rule applies
	auto inner = f.scripts( true, false );
	const float inner_height = layout_extent( *inner ).max.y();

	auto outer = std::make_unique<Scripts>( f.font, f.s, std::move( inner ), f.text( "3", ScriptLevel::SCRIPT ), nullptr );
	auto it = outer->begin();
	++it;
	// u >= height of nucleus - superscriptBaselineDropMax
	CHECK( baseline_of( **it ) >= inner_height - f.font.superscript_baseline_drop_max() * f.s - 1e-4F );
}

TEST_CASE( "Scripts scale as a unit", "[visuals]" )
{
	Fixture f;
	auto scripts = f.scripts( true, true );
	const AABB before = *scripts->extent;
	*scripts *= 2.F;
	CHECK( scripts->extent->max.x() == Approx( before.max.x() * 2.F ) );
	CHECK( scripts->extent->max.y() == Approx( before.max.y() * 2.F ) );
	CHECK( scripts->extent->min.y() == Approx( before.min.y() * 2.F ) );
}

TEST_CASE( "Math text carries italic corrections and Scripts uses them", "[visuals]" )
{
	auto theme = test_theme();
	FontStyle body = FontStyle{ Intent::MATH }.large();
	MathFonts math_fonts = theme.get_math( body );
	MathFont font( math_fonts.roman() );
	const float s = math_fonts.roman().scale / static_cast<float>( math_fonts.roman().face->units_per_EM );

	auto nucleus = make_text( theme, "f", body );
	const Glyph *glyph = last_glyph( *nucleus );
	REQUIRE( glyph != nullptr );

	SECTION( "an italic letter maps to the math alphabet and takes its correction from the font" )
	{
		CHECK( glyph->index == font.get_glyph_index( 0x1D453 ) );
		CHECK( glyph->italicCorrection > 0.F );
		CHECK( glyph->italicCorrection == Approx( font.italic_correction( glyph->index ) * s ) );
	}

	SECTION( "text in a non-math font has none" )
	{
		auto prose = make_text( theme, "f", FontStyle{ Intent::PROSE } );
		CHECK( last_glyph( *prose )->italicCorrection == 0.F );
	}

	SECTION( "the superscript clears the nucleus by that correction; the subscript does not" )
	{
		auto script = script_style( body, font, ScriptLevel::SCRIPT );
		const float italic = glyph->italicCorrection;
		Scripts scripts(
			font, s, std::move( nucleus ), make_text( theme, "2", script ), make_text( theme, "i", script ) );

		auto it = scripts.begin();
		const AABB n = layout_extent( **it++ );
		const AABB sup = layout_extent( **it++ );
		const AABB sub = layout_extent( **it );
		CHECK( sup.min.x() == Approx( n.max.x() + italic ) );
		CHECK( sub.min.x() == Approx( n.max.x() ) );
	}
}
