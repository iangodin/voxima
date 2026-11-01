#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "RenderTestUtil.h"
#include "TestThemes.h"

#include <core/Animate.h>
#include <core/Stage.h>
#include <visuals/CallbackVisitor.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Glyph.h>
#include <visuals/Gradient.h>
#include <visuals/Line.h>
#include <visuals/Morph.h>
#include <visuals/Outline.h>
#include <visuals/Rectangle.h>
#include <visuals/Tags.h>

#include <cmath>
#include <memory>
#include <string>
#include <vector>

using Catch::Approx;
using namespace render_test;

namespace
{
const Color BLACK{ 0.F, 0.F, 0.F, 1.F };
const Color RED{ 1.F, 0.F, 0.F, 1.F };
const Color BLUE{ 0.F, 0.F, 1.F, 1.F };
// (not pure white, which stands for the text color)
const Color BRIGHT{ 1.F, 1.F, 0.F, 1.F };

bool contains( const std::string &text, const std::string &needle )
{
	return text.find( needle ) != std::string::npos;
}

size_t count( const std::string &text, const std::string &needle )
{
	size_t n = 0;
	for ( size_t at = text.find( needle ); at != std::string::npos; at = text.find( needle, at + needle.size() ) )
	{
		++n;
	}
	return n;
}

// 4 by 4, black at the bottom and red at the top
Rectangle upward_box()
{
	Rectangle box( 2.F, 2.F );
	box.fill_gradient = Gradient::linear( { 0.F, -2.F }, { 0.F, 2.F }, { { 0.F, BLACK }, { 1.F, RED } } );
	return box;
}
}

TEST_CASE( "A linear gradient goes from its start to its end, y up", "[gradient]" )
{
	const auto picture = render( upward_box() );
	CHECK( pixel_at( picture, 0.F, -1.99F ).r < 5 );
	CHECK( pixel_at( picture, 0.F, 0.F ).r == Approx( 127 ).margin( 2 ) );
	CHECK( pixel_at( picture, 0.F, 1.98F ).r > 250 );
	CHECK( pixel_at( picture, 3.F, 0.F ).r == 0 ); // and it fills only the shape
}

TEST_CASE( "A gradient moves, turns and scales with its visual", "[gradient]" )
{
	Rectangle box = upward_box();
	box.position = { 3.F, 0.F };
	CHECK( pixel_at( render( box ), 3.F, 1.9F ).r > 240 );

	box.rotation = 90.F; // what was up is now to the left
	const auto turned = render( box );
	CHECK( pixel_at( turned, 1.1F, 0.F ).r > 240 );
	CHECK( pixel_at( turned, 4.9F, 0.F ).r < 15 );

	Rectangle scaled = upward_box();
	scaled *= 0.5F; // its coordinates change, so the gradient's must
	const auto picture = render( scaled );
	CHECK( pixel_at( picture, 0.F, 0.97F ).r > 240 );
	CHECK( pixel_at( picture, 0.F, -0.97F ).r < 15 );
}

TEST_CASE( "A radial gradient goes from its focus to its circle", "[gradient]" )
{
	Circle disc( 2.F );
	disc.fill_gradient = Gradient::radial( { 0.F, 0.F }, 2.F, { { 0.F, RED }, { 1.F, BLACK } } );
	auto picture = render( disc );
	CHECK( pixel_at( picture, 0.F, 0.F ).r > 250 );
	CHECK( pixel_at( picture, 1.F, 0.F ).r == Approx( 127 ).margin( 3 ) );

	disc.fill_gradient = Gradient::radial( { 0.F, 0.F }, 2.F, { 1.F, 0.F }, { { 0.F, RED }, { 1.F, BLACK } } );
	picture = render( disc );
	CHECK( pixel_at( picture, 1.F, 0.F ).r > 250 );
	CHECK( pixel_at( picture, -1.F, 0.F ).r < pixel_at( picture, 0.F, 0.F ).r );
}

TEST_CASE( "A gradient is tinted and faded by the visual's color and its groups'", "[gradient]" )
{
	SECTION( "fading" )
	{
		Group group;
		group.color.a = 0.5F;
		auto &box = group.add( std::make_unique<Rectangle>( 2.F, 2.F ) );
		box.fill_gradient = Gradient::linear( { 0.F, -2.F }, { 0.F, 2.F }, { { 0.F, RED }, { 1.F, RED } } );
		const auto picture = render( group );
		CHECK( pixel_at( picture, 0.F, 0.F ).r == Approx( 127 ).margin( 2 ) );
		// A fade keeps the one definition, so a fade does not write a new gradient at each alpha
		CHECK( contains( svg_of( group ), "fill-opacity=\"0.5" ) );
		CHECK_FALSE( contains( svg_of( group ), "stop-opacity" ) );
	}
	SECTION( "tinting" )
	{
		Rectangle box( 2.F, 2.F );
		box.color = BLUE;
		box.fill_gradient = Gradient::linear( { 0.F, -2.F }, { 0.F, 2.F }, { { 0.F, Color{ 1.F, 0.5F, 1.F, 1.F } }, { 1.F, Color{ 1.F, 0.5F, 1.F, 1.F } } } );
		const auto pixel = pixel_at( render( box ), 0.F, 0.F );
		CHECK( pixel.r == 0 );
		CHECK( pixel.g == 0 );
		CHECK( pixel.b == 255 );
	}
	SECTION( "a stop's own alpha" )
	{
		Rectangle box( 2.F, 2.F );
		box.fill_gradient = Gradient::linear( { 0.F, -2.F }, { 0.F, 2.F }, { { 0.F, RED }, { 1.F, Color{ 1.F, 0.F, 0.F, 0.F } } } );
		const auto picture = render( box );
		CHECK( pixel_at( picture, 0.F, 0.F ).r == Approx( 127 ).margin( 3 ) );
		CHECK( pixel_at( picture, 0.F, 1.98F ).r < 8 );
	}
}

TEST_CASE( "White in a gradient is the theme's text color", "[gradient]" )
{
	std::ostringstream body;
	SVGWriter writer( body, 0, 0.F, Color{ 0.2F, 0.4F, 0.6F, 1.F } );
	Rectangle box( 1.F, 1.F );
	box.fill_gradient = Gradient::linear( { -1.F, 0.F }, { 1.F, 0.F }, { { 0.F, Color{} }, { 1.F, BLACK } } );
	box.accept( writer );
	std::ostringstream defs;
	writer.write_defs( defs );
	CHECK( contains( defs.str(), "stop-color=\"rgb(51,102,153)\"" ) );
	CHECK( contains( defs.str(), "stop-color=\"rgb(0,0,0)\"" ) );
}

TEST_CASE( "The same gradient is written once, and a gradient is on what it fills", "[gradient]" )
{
	Group group;
	for ( int i = 0; i < 3; ++i )
	{
		auto &box = group.add( std::make_unique<Rectangle>( 1.F, 1.F ) );
		box.fill_gradient = Gradient::linear( { -1.F, 0.F }, { 1.F, 0.F }, { { 0.F, RED }, { 1.F, BLUE } } );
	}
	auto &other = group.add( std::make_unique<Rectangle>( 1.F, 1.F ) );
	other.fill_gradient = Gradient::linear( { -1.F, 0.F }, { 1.F, 0.F }, { { 0.F, RED }, { 1.F, BLACK } } );
	const std::string svg = svg_of( group );
	CHECK( count( svg, "<linearGradient" ) == 2 );
	CHECK( count( svg, "url(#gradient_0)" ) == 3 );
	CHECK( count( svg, "url(#gradient_1)" ) == 1 );
}

TEST_CASE( "Without a gradient, nothing is written that was not before", "[gradient]" )
{
	const Rectangle box( 1.F, 1.F );
	CHECK_FALSE( contains( svg_of( box ), "radient" ) );
}

TEST_CASE( "Every shape takes a gradient", "[gradient]" )
{
	Gradient gradient = Gradient::linear( { -1.F, 0.F }, { 1.F, 0.F }, { { 0.F, RED }, { 1.F, RED } } );
	const auto red_at_centre = [&]( Visual &shape )
	{
		shape.fill_gradient = gradient;
		return pixel_at( render( shape ), 0.F, 0.F ).r;
	};
	Circle circle( 1.F );
	CHECK( red_at_centre( circle ) == 255 );
	Rectangle rectangle( 1.F, 1.F );
	CHECK( red_at_centre( rectangle ) == 255 );
	Path path;
	path.commands = { MoveTo{ { -1.F, -1.F } }, LineTo{ { 1.F, -1.F } }, LineTo{ { 0.F, 1.F } }, Close{} };
	CHECK( red_at_centre( path ) == 255 );
	Glyph glyph;
	glyph.commands = path.commands;
	CHECK( red_at_centre( glyph ) == 255 );
}

TEST_CASE( "A dashed circle takes a gradient, too", "[gradient][dash]" )
{
	Circle circle( 1.F );
	circle.stroke_thickness = 0.1F;
	circle.stroke_dash = StrokeDash::dashed( 0.2F, 0.2F );
	circle.fill_gradient = Gradient::linear( { -1.F, 0.F }, { 1.F, 0.F }, { { 0.F, RED }, { 1.F, RED } } );
	CHECK( pixel_at( render( circle ), 0.F, 0.F ).r == 255 );
}

TEST_CASE( "A gradient that goes nowhere is its last color, and with no stops it is the plain fill", "[gradient]" )
{
	const std::vector<GradientStop> stops{ { 0.F, BLUE }, { 1.F, RED } };
	Rectangle box( 2.F, 2.F, BRIGHT );

	const auto seen = [&]( const Gradient &gradient )
	{
		box.fill_gradient = gradient;
		return pixel_at( render( box ), 0.F, 0.F );
	};
	Rectangle plain( 1.F, 1.F );
	CHECK( seen( Gradient::linear( { 1.F, 1.F }, { 1.F, 1.F }, stops ) ).b == 0 );
	CHECK( seen( Gradient::radial( { 0.F, 0.F }, 0.F, stops ) ).r > 0 );

	Gradient flat = Gradient::linear( { -1.F, 0.F }, { 1.F, 0.F }, stops );
	flat.transform = { 0.F, 0.F, 0.F, 0.F, 0.F, 0.F };
	CHECK_FALSE( flat.drawable() );
	CHECK( seen( flat ).b == 0 );

	box.color = { 1.F, 1.F, 1.F, 1.F };
	const auto last = seen( Gradient::linear( { 1.F, 1.F }, { 1.F, 1.F }, stops ) );
	CHECK( last.r == 255 );
	CHECK( last.b == 0 );

	box.color = BRIGHT;
	box.fill_gradient = Gradient{};
	const auto none = pixel_at( render( box ), 0.F, 0.F );
	CHECK( none.r == 255 );
	CHECK( none.g == 255 );
}

TEST_CASE( "Stops are put in order and kept in range", "[gradient]" )
{
	const Gradient gradient = Gradient::linear( { 0.F, 0.F }, { 1.F, 0.F }, { { 2.F, RED }, { 0.5F, BLUE }, { -1.F, BLACK } } );
	const auto stops = gradient.ordered_stops();
	REQUIRE( stops.size() == 3 );
	CHECK( stops[0].offset == 0.F );
	CHECK( stops[0].color == BLACK );
	CHECK( stops[1].offset == 0.5F );
	CHECK( stops[2].offset == 1.F );
	CHECK( stops[2].color == RED );
}

TEST_CASE( "A gradient has a color anywhere, and comes to an average", "[gradient]" )
{
	const Gradient gradient = Gradient::linear( { 0.F, 0.F }, { 1.F, 0.F }, { { 0.25F, RED }, { 0.75F, BLUE } } );
	CHECK( gradient.sample( 0.F ) == RED );
	CHECK( gradient.sample( 1.F ) == BLUE );
	CHECK( gradient.sample( 0.5F ).r == Approx( 0.5F ) );
	CHECK( gradient.sample( 0.5F ).b == Approx( 0.5F ) );
	CHECK( gradient.average().r == Approx( 0.5F ) );
	CHECK( gradient.average().b == Approx( 0.5F ) );
	CHECK( gradient.average().a == Approx( 1.F ) );

	const Gradient lopsided = Gradient::linear( { 0.F, 0.F }, { 1.F, 0.F }, { { 0.F, RED }, { 0.5F, RED }, { 1.F, BLUE } } );
	CHECK( lopsided.average().r == Approx( 0.75F ) );
}

TEST_CASE( "Gradient::across reaches the corners of the box", "[gradient]" )
{
	AABB box;
	box.add( { -2.F, -1.F } );
	box.add( { 2.F, 1.F } );
	const std::vector<GradientStop> stops{ { 0.F, BLACK }, { 1.F, RED } };

	const Gradient up = Gradient::across( box, 90.F, stops );
	CHECK( up.start.x() == Approx( 0.F ).margin( 1e-5 ) );
	CHECK( up.start.y() == Approx( -1.F ) );
	CHECK( up.end.y() == Approx( 1.F ) );

	const Gradient right = Gradient::across( box, 0.F, stops );
	CHECK( right.start.x() == Approx( -2.F ) );
	CHECK( right.end.x() == Approx( 2.F ) );

	// On the slant, the corner is exactly where the color starts
	const Gradient slant = Gradient::across( box, 45.F, stops );
	const Point heading = ( slant.end - slant.start ) * ( 1.F / std::sqrt( ( slant.end - slant.start ).lengthSquared() ) );
	const Point corner{ -2.F, -1.F };
	CHECK( ( corner - slant.start ).x() * heading.x() + ( corner - slant.start ).y() * heading.y() == Approx( 0.F ).margin( 1e-5 ) );
}

TEST_CASE( "fill_each_with_gradient gives each glyph its own", "[gradient]" )
{
	Theme theme = test_theme();
	auto text = make_text( theme, "AB", FontStyle{ Intent::TITLE } );
	const std::vector<GradientStop> stops{ { 0.F, RED }, { 1.F, BLUE } };

	auto &line = text->add( std::make_unique<Line>() );
	line.stroke_thickness = 0.1F;
	fill_each_with_gradient( *text, 90.F, stops );

	std::vector<const Gradient *> gradients;
	CallbackVisitor collect(
		[&]( Visual &leaf )
		{
			if ( dynamic_cast<Glyph *>( &leaf ) )
			{
				REQUIRE( leaf.fill_gradient );
				gradients.push_back( &*leaf.fill_gradient );
			}
		} );
	text->accept( collect );
	REQUIRE( gradients.size() == 2 );
	CHECK( gradients[0]->start.y() < gradients[0]->end.y() );
	CHECK_FALSE( line.fill_gradient );

	// and it covers that glyph: the ink of the first runs from about where its gradient starts to where it ends
	const auto shapes = to_outline( *text );
	CHECK( shapes[0].gradient );
	const AABB ink = on_curve_bounds( shapes[0].contours[0] );
	CHECK( gradients[0]->start.y() == Approx( ink.min.y() ).margin( ink.max.y() - ink.min.y() ) );
}

TEST_CASE( "Outlines keep a gradient in their own frame and colors", "[gradient]" )
{
	Group group;
	group.position = { 1.F, 2.F };
	group.scale_x = group.scale_y = 2.F;
	group.color = Color{ 1.F, 1.F, 1.F, 0.5F };
	auto &box = group.add( std::make_unique<Rectangle>( 1.F, 1.F ) );
	box.fill_gradient = Gradient::linear( { -1.F, 0.F }, { 1.F, 0.F }, { { 0.F, RED }, { 1.F, BLUE } } );

	const auto shapes = to_outline( group );
	REQUIRE( shapes.size() == 1 );
	REQUIRE( shapes[0].gradient );
	CHECK( shapes[0].gradient->transform.a == Approx( 2.F ) );
	CHECK( shapes[0].gradient->transform.tx == Approx( 1.F ) );
	CHECK( shapes[0].gradient->stops[0].color.a == Approx( 0.5F ) );
	CHECK( shapes[0].fill.r == Approx( 0.5F ) ); // what it comes to overall
	CHECK( shapes[0].fill.a == Approx( 0.5F ) );
}

TEST_CASE( "A morph starts and ends looking as its gradients do", "[gradient][morph]" )
{
	Group from;
	from.position = { -1.F, 0.5F };
	from.rotation = 30.F;
	from.scale_x = 1.5F;
	auto &box = from.add( std::make_unique<Rectangle>( 1.F, 1.F ) );
	box.fill_gradient = Gradient::linear( { -1.F, -1.F }, { 1.F, 1.F }, { { 0.F, RED }, { 1.F, BLUE } } );

	Circle to( 1.5F );
	to.position = { 2.F, 0.F };
	to.fill_gradient = Gradient::radial( { 0.F, 0.F }, 1.5F, { { 0.F, BLACK }, { 1.F, RED } } );

	Morph morph( from, to );
	morph.set( 0.F );
	CHECK( pixels_apart( render( morph.visual() ), render( from ), 24 ) < 40 );
	morph.set( 1.F );
	CHECK( pixels_apart( render( morph.visual() ), render( to ), 24 ) < 120 );
}

TEST_CASE( "A morph turns a plain fill into a gradient, and back", "[gradient][morph]" )
{
	const Rectangle plain( 2.F, 2.F, BLUE );
	const Rectangle fancy = upward_box();

	Morph there( plain, fancy );
	there.set( 0.F );
	CHECK_FALSE( ( *there.visual().begin() )->fill_gradient );
	there.set( 0.5F );
	const auto halfway = render( there.visual() );
	// Black and blue at the bottom; red and blue at the top
	CHECK( pixel_at( halfway, 0.F, -1.9F ).r < 20 );
	CHECK( pixel_at( halfway, 0.F, -1.9F ).b == Approx( 127 ).margin( 3 ) );
	CHECK( pixel_at( halfway, 0.F, 1.9F ).r > 110 );
	CHECK( pixel_at( halfway, 0.F, 1.9F ).b == Approx( 127 ).margin( 3 ) );
	there.set( 1.F );
	CHECK( ( *there.visual().begin() )->fill_gradient );

	Morph back( fancy, plain );
	back.set( 1.F );
	CHECK_FALSE( ( *back.visual().begin() )->fill_gradient );
	CHECK( ( *back.visual().begin() )->color == BLUE );
}

TEST_CASE( "A morph blends two gradients of a kind, stops and places", "[gradient][morph]" )
{
	Rectangle a( 2.F, 2.F );
	a.fill_gradient = Gradient::linear( { 0.F, -2.F }, { 0.F, 2.F }, { { 0.F, BLACK }, { 1.F, RED } } );
	Rectangle b( 2.F, 2.F );
	b.position = { 0.F, 0.001F }; // (otherwise it is the same shape)
	b.fill_gradient = Gradient::linear( { 0.F, 2.F }, { 0.F, -2.F }, { { 0.F, BLUE }, { 0.5F, BLACK }, { 1.F, BLUE } } );

	Morph morph( a, b );
	morph.set( 0.5F );
	const auto &gradient = *( *morph.visual().begin() )->fill_gradient;
	REQUIRE( gradient.stops.size() == 3 ); // stops wherever either has one
	CHECK( gradient.stops[1].offset == 0.5F );
	CHECK( gradient.stops[1].color.r == Approx( 0.25F ) ); // halfway from red's 0.5 point to black
	CHECK( gradient.stops[2].color.r == Approx( 0.5F ) );
	CHECK( gradient.stops[2].color.b == Approx( 0.5F ) );
}

TEST_CASE( "A morph between a linear and a radial gradient goes through the average color", "[gradient][morph]" )
{
	Rectangle a( 2.F, 2.F );
	a.fill_gradient = Gradient::linear( { 0.F, -2.F }, { 0.F, 2.F }, { { 0.F, RED }, { 1.F, RED } } );
	Rectangle b( 2.F, 2.F );
	b.position = { 0.F, 0.001F };
	b.fill_gradient = Gradient::radial( { 0.F, 0.F }, 2.F, { { 0.F, BLUE }, { 1.F, BLUE } } );

	Morph morph( a, b );
	const auto gradient_of = [&]() -> const Gradient & { return *( *morph.visual().begin() )->fill_gradient; };

	morph.set( 0.25F );
	CHECK( gradient_of().kind == GradientKind::LINEAR );
	morph.set( 0.75F );
	CHECK( gradient_of().kind == GradientKind::RADIAL );

	// At the middle, a plain color: the same from either side
	morph.set( 0.4999F );
	const auto before = gradient_of().stops.front().color;
	morph.set( 0.5F );
	const auto after = gradient_of().stops.front().color;
	CHECK( before.r == Approx( 0.5F ).margin( 1e-3 ) );
	CHECK( after.r == Approx( 0.5F ).margin( 1e-3 ) );
	CHECK( after.b == Approx( 0.5F ).margin( 1e-3 ) );
}

TEST_CASE( "What fades out or in during a morph fades its gradient", "[gradient][morph]" )
{
	Group a;
	auto &first = a.add( std::make_unique<Rectangle>( 1.F, 1.F ) );
	first.fill_gradient = Gradient::linear( { -1.F, 0.F }, { 1.F, 0.F }, { { 0.F, RED }, { 1.F, RED } } );
	Group b;

	Morph morph( a, b );
	morph.set( 0.5F );
	CHECK( pixel_at( render( morph.visual() ), 0.F, 0.F ).r == Approx( 127 ).margin( 3 ) );
	morph.set( 1.F );
	CHECK( pixel_at( render( morph.visual() ), 0.F, 0.F ).r == 0 );
}

TEST_CASE( "A shape carried by a morph carries its gradient", "[gradient][morph]" )
{
	// The same shape, turned: the part is carried whole, and halfway it must look like the shape turned halfway
	Rectangle a = upward_box();
	a.position = { 0.5F, 0.F };
	Rectangle b = upward_box();
	b.position = { 0.5F, 0.F };
	b.rotation = 90.F;

	Morph morph( a, b );
	morph.set( 0.5F );
	Rectangle halfway = upward_box();
	halfway.position = { 0.5F, 0.F };
	halfway.rotation = 45.F;
	CHECK( pixels_apart( render( morph.visual() ), render( halfway ), 32 ) < 200 );
}
