#include <catch2/catch_test_macros.hpp>

#include <core/MorphAnim.h>
#include <core/EaseLinear.h>
#include <core/Stage.h>
#include <exporters/SVGWriter.h>
#include <visuals/Circle.h>
#include <visuals/Group.h>

#include <chrono>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::chrono_literals;

namespace
{
void run( Scene scene )
{
	scene.resume();
	while ( !scene.done() )
	{
		scene.resume();
	}
}

std::vector<const Visual *> children( const Group &group )
{
	std::vector<const Visual *> out;
	for ( const auto &child: group )
	{
		out.push_back( child.get() );
	}
	return out;
}

// Three children: a, b and c, from back to front
struct Three
{
	Group group;
	Circle *a = &group.add( std::make_unique<Circle>() );
	Circle *b = &group.add( std::make_unique<Circle>() );
	Circle *c = &group.add( std::make_unique<Circle>() );

	std::vector<const Visual *> order() const
	{
		return children( group );
	}

	std::vector<const Visual *> abc( const Visual *x, const Visual *y, const Visual *z ) const
	{
		return { x, y, z };
	}
};

std::string svg_of( const Visual &visual, const Color &foreground )
{
	std::ostringstream out;
	SVGWriter writer( out, 0, 0.F, foreground );
	visual.accept( writer );
	return out.str();
}
}

TEST_CASE( "Group::insert puts a child at a place in the drawing order", "[group]" )
{
	Three t;
	auto &back = t.group.insert( 0, std::make_unique<Circle>() );
	auto &middle = t.group.insert( 2, std::make_unique<Circle>() );
	auto &front = t.group.insert( 1000, std::make_unique<Circle>() );
	CHECK( children( t.group ) == std::vector<const Visual *>{ &back, t.a, &middle, t.b, t.c, &front } );
}

TEST_CASE( "A child can be moved in the drawing order", "[group]" )
{
	Three t;
	SECTION( "to the front" )
	{
		CHECK( t.group.to_front( t.a ) );
		CHECK( t.order() == t.abc( t.b, t.c, t.a ) );
	}
	SECTION( "to the back" )
	{
		CHECK( t.group.to_back( t.c ) );
		CHECK( t.order() == t.abc( t.c, t.a, t.b ) );
	}
	SECTION( "one forward" )
	{
		CHECK( t.group.raise( t.a ) );
		CHECK( t.order() == t.abc( t.b, t.a, t.c ) );
	}
	SECTION( "one back" )
	{
		CHECK( t.group.lower( t.c ) );
		CHECK( t.order() == t.abc( t.a, t.c, t.b ) );
	}
	SECTION( "past either end stays where it is" )
	{
		CHECK( t.group.raise( t.c ) );
		CHECK( t.group.lower( t.a ) );
		CHECK( t.group.to_front( t.c ) );
		CHECK( t.group.to_back( t.a ) );
		CHECK( t.order() == t.abc( t.a, t.b, t.c ) );
	}
	SECTION( "what is not a direct child changes nothing" )
	{
		Circle other;
		CHECK_FALSE( t.group.to_front( &other ) );
		CHECK_FALSE( t.group.to_back( &other ) );
		CHECK_FALSE( t.group.raise( &other ) );
		CHECK_FALSE( t.group.lower( &other ) );
		CHECK( t.order() == t.abc( t.a, t.b, t.c ) );
	}
}

TEST_CASE( "A morph can be in any group, and keeps its place there", "[group][morph]" )
{
	Stage stage;
	auto &layer = stage.visuals.add( std::make_unique<Group>() );
	auto &behind = layer.add( std::make_unique<Circle>() );
	auto &from = layer.add( std::make_unique<Circle>( 1.F ) );
	auto &front = layer.add( std::make_unique<Circle>() );
	auto to = std::make_unique<Circle>( 2.F );
	const Circle *to_ptr = to.get();

	MorphOptions options;
	options.parent = &layer;
	run( morph( stage, from, std::move( to ), 0.1s, ease_linear, options ) );

	CHECK( children( layer ) == std::vector<const Visual *>{ &behind, to_ptr, &front } );
	CHECK( stage.visuals.size() == 1 );
}

TEST_CASE( "A morph of what is not a child of the group is refused", "[group][morph]" )
{
	Stage stage;
	auto &layer = stage.visuals.add( std::make_unique<Group>() );
	auto &from = layer.add( std::make_unique<Circle>() );

	CHECK_THROWS_AS( run( morph( stage, from, std::make_unique<Circle>(), 0.1s, ease_linear ) ), std::invalid_argument );

	Group other;
	MorphOptions options;
	options.parent = &other;
	CHECK_THROWS_AS( run( morph( stage, from, std::make_unique<Circle>(), 0.1s, ease_linear, options ) ), std::invalid_argument );
	CHECK( layer.size() == 1 );
}

TEST_CASE( "A morph in a tinted group starts in the color it was", "[group][morph]" )
{
	const Color foreground{ 0.5F, 0.5F, 0.5F, 1.F };
	Stage stage;
	stage.foreground = foreground;
	auto &layer = stage.visuals.add( std::make_unique<Group>() );
	layer.color = { 1.F, 0.F, 0.F, 1.F };
	auto &from = layer.add( std::make_unique<Circle>( 1.F ) ); // white: drawn as the tint
	CHECK( svg_of( layer, foreground ).find( "rgb(255,0,0)" ) != std::string::npos );

	MorphOptions options;
	options.parent = &layer;
	Scene scene = morph( stage, from, std::make_unique<Circle>( 2.F ), 1s, ease_linear, options );
	scene.resume();
	CHECK( svg_of( layer, foreground ).find( "rgb(255,0,0)" ) != std::string::npos );
	while ( !scene.done() )
	{
		scene.resume();
	}
}
