#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <visuals/Accent.h>
#include <visuals/Bars.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Glyph.h>
#include <visuals/Group.h>
#include <visuals/Layout.h>
#include <visuals/MathMetrics.h>
#include <visuals/Rectangle.h>

#include <algorithm>
#include "TestThemes.h"

using Catch::Approx;

namespace
{
struct Fixture
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

	// A stand-in for a character: a box from the origin, `width` wide and `height` tall,
	// with a top accent attachment
	static std::unique_ptr<Glyph> character( float attach, float height = 0.1F, float width = 1.F )
	{
		auto glyph = std::make_unique<Glyph>();
		AABB box;
		box.add( { 0.F, 0.F } );
		box.add( { width, height } );
		glyph->extent = box;
		glyph->topAccentAttachment = attach;
		return glyph;
	}
};

const Glyph *accent_glyph( const Accent &accent )
{
	auto it = accent.begin();
	++it;
	return dynamic_cast<const Glyph *>( it->get() );
}
}

TEST_CASE( "top_accent_x", "[visuals]" )
{
	SECTION( "a glyph with an attachment, in its parent's frame" )
	{
		Glyph glyph;
		glyph.position = { 1.F, 0.F };
		glyph.topAccentAttachment = 0.3F;
		REQUIRE( top_accent_x( glyph ).has_value() );
		CHECK( *top_accent_x( glyph ) == Approx( 1.3F ) );
	}

	SECTION( "a glyph the font says nothing about" )
	{
		Glyph glyph;
		CHECK_FALSE( top_accent_x( glyph ).has_value() );
	}

	SECTION( "a group holding one character passes it through with its own offset" )
	{
		Group group;
		group.position = { 2.F, 0.F };
		auto &inner = group.add( std::make_unique<Glyph>() );
		inner.position = { 1.F, 0.F };
		inner.topAccentAttachment = 0.4F;
		REQUIRE( top_accent_x( group ).has_value() );
		CHECK( *top_accent_x( group ) == Approx( 3.4F ) );
	}

	SECTION( "several characters, a composite or a plain shape have none" )
	{
		Group two;
		two.add( std::make_unique<Glyph>() ).topAccentAttachment = 0.1F;
		two.add( std::make_unique<Glyph>() ).topAccentAttachment = 0.1F;
		CHECK_FALSE( top_accent_x( two ).has_value() );

		Group composite;
		composite.add( std::make_unique<Glyph>() ).topAccentAttachment = 0.1F;
		AABB box;
		box.add( { 0.F, 0.F } );
		composite.extent = box;
		CHECK_FALSE( top_accent_x( composite ).has_value() );

		Circle circle;
		CHECK_FALSE( top_accent_x( circle ).has_value() );
	}
}

TEST_CASE( "Accent placement", "[visuals]" )
{
	Fixture f;
	const float accent_base = f.font.accent_base_height() * f.s;
	const char32_t HAT = 0x2C6;

	SECTION( "the accent's attachment point sits over the base's" )
	{
		Accent accent( f.font, f.s, Fixture::character( 0.3F ), HAT );
		REQUIRE( accent.size() == 2 );
		const Glyph *glyph = accent_glyph( accent );
		REQUIRE( glyph != nullptr );
		REQUIRE( glyph->topAccentAttachment.has_value() );
		CHECK( glyph->position.x() + *glyph->topAccentAttachment == Approx( 0.3F ) );
	}

	SECTION( "without an attachment it is centered over the base's box" )
	{
		Accent accent( f.font, f.s, Fixture::circle( 0.5F ), HAT );
		const Glyph *glyph = accent_glyph( accent );
		CHECK( glyph->position.x() + *glyph->topAccentAttachment == Approx( 0.5F ) );
	}

	SECTION( "a base no taller than the accent height leaves the accent where it is drawn" )
	{
		const float low = accent_base * 0.5F;
		Accent accent( f.font, f.s, Fixture::character( 0.5F, low ), HAT );
		CHECK( accent_glyph( accent )->position.y() == Approx( 0.F ) );
	}

	SECTION( "a taller base raises the accent by the difference" )
	{
		Accent accent( f.font, f.s, Fixture::character( 0.5F, 1.F ), HAT );
		CHECK( accent_glyph( accent )->position.y() == Approx( 1.F - accent_base ) );
	}

	SECTION( "the base is measured from its own baseline" )
	{
		auto base = Fixture::character( 0.5F, 1.F );
		base->position = { 3.F, 2.F }; // sits two units up
		base->baseline = 0.F;
		Accent accent( f.font, f.s, std::move( base ), HAT );

		const Visual &placed = **accent.begin();
		CHECK( placed.position.x() == Approx( 0.F ) ); // left edge at the origin
		CHECK( placed.position.y() == Approx( 0.F ) ); // baseline at y = 0
		CHECK( accent_glyph( accent )->position.y() == Approx( 1.F - accent_base ) );
	}

	SECTION( "the box is as wide as the base; the accent may overhang" )
	{
		Accent accent( f.font, f.s, Fixture::character( 0.5F, 0.1F, 2.F ), HAT );
		REQUIRE( accent.extent.has_value() );
		CHECK( accent.extent->min.x() == Approx( 0.F ) );
		CHECK( accent.extent->max.x() == Approx( 2.F ) );
		CHECK( accent.extent->max.y() >= layout_extent( *accent_glyph( accent ) ).max.y() - 1e-4F );
	}

	SECTION( "text uses the attachment the math font gives its glyph" )
	{
		auto base = make_text( f.theme, "x", FontStyle{ Intent::MATH } );
		REQUIRE( top_accent_x( *base ).has_value() );
		const float expected = *top_accent_x( *base ) - layout_extent( *base ).min.x();

		Accent accent( f.font, f.s, std::move( base ), HAT );
		const Glyph *glyph = accent_glyph( accent );
		CHECK( glyph->position.x() + *glyph->topAccentAttachment == Approx( expected ) );
	}

	SECTION( "a combining accent, drawn to the left of its origin, is still centered" )
	{
		Accent accent( f.font, f.s, Fixture::character( 0.4F ), 0x20D7 );
		const Glyph *glyph = accent_glyph( accent );
		CHECK( glyph->position.x() + *glyph->topAccentAttachment == Approx( 0.4F ) );
	}
}

TEST_CASE( "Overline", "[visuals]" )
{
	Fixture f;
	const float gap = f.font.overbar_vertical_gap() * f.s;
	const float thickness = f.font.overbar_rule_thickness() * f.s;
	const float extra = f.font.overbar_extra_ascender() * f.s;

	Overline over( f.font, f.s, Fixture::circle( 0.5F ) );
	REQUIRE( over.size() == 2 );
	auto it = over.begin();
	++it;
	const auto *bar = dynamic_cast<const Rectangle *>( it->get() );
	REQUIRE( bar != nullptr );

	CHECK( bar->half_height * 2.F == Approx( thickness ) );
	CHECK( bar->half_width * 2.F == Approx( 1.F ) );
	CHECK( bar->position.x() == Approx( 0.5F ) );
	// its bottom is the font's gap above the content's top
	CHECK( bar->position.y() - thickness / 2.F == Approx( 0.5F + gap ) );

	REQUIRE( over.extent.has_value() );
	CHECK( over.extent->min.x() == Approx( 0.F ) );
	CHECK( over.extent->max.x() == Approx( 1.F ) );
	CHECK( over.extent->min.y() == Approx( -0.5F ) );
	CHECK( over.extent->max.y() == Approx( 0.5F + gap + thickness + extra ) );
}

TEST_CASE( "Underline", "[visuals]" )
{
	Fixture f;
	const float gap = f.font.underbar_vertical_gap() * f.s;
	const float thickness = f.font.underbar_rule_thickness() * f.s;
	const float extra = f.font.underbar_extra_descender() * f.s;

	Underline under( f.font, f.s, Fixture::circle( 0.5F ) );
	REQUIRE( under.size() == 2 );
	auto it = under.begin();
	++it;
	const auto *bar = dynamic_cast<const Rectangle *>( it->get() );
	REQUIRE( bar != nullptr );

	CHECK( bar->half_height * 2.F == Approx( thickness ) );
	CHECK( bar->half_width * 2.F == Approx( 1.F ) );
	// its top is the font's gap below the content's bottom
	CHECK( bar->position.y() + thickness / 2.F == Approx( -0.5F - gap ) );

	REQUIRE( under.extent.has_value() );
	CHECK( under.extent->max.y() == Approx( 0.5F ) );
	CHECK( under.extent->min.y() == Approx( -0.5F - gap - thickness - extra ) );
}

TEST_CASE( "Bars scale as a unit", "[visuals]" )
{
	Fixture f;
	Overline over( f.font, f.s, Fixture::circle( 0.5F ) );
	const AABB before = *over.extent;
	over *= 2.F;
	CHECK( over.extent->max.y() == Approx( before.max.y() * 2.F ) );
	CHECK( over.extent->max.x() == Approx( before.max.x() * 2.F ) );
}
