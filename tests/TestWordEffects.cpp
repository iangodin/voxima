#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <core/WordEffects.h>
#include <themes/NewComputerModern.h>
#include <visuals/CreateText.h>
#include <visuals/Rectangle.h>
#include <visuals/Tags.h>

#include <vector>

using Catch::Approx;

namespace
{
// Three words of laid-out text and a clock the test moves by hand
struct Fixture
{
	Theme theme = NewComputerModern::create_theme();
	std::unique_ptr<Group> text = make_text( theme, "one two three", FontStyle{ Intent::PROSE } );
	double now = 0.0;
	bool over = false;

	// The i-th word, said from `start` to `end`
	WordContext word( size_t i, double start, double end )
	{
		return WordContext{ *text, *find_word( *text, i ), i, start, end, [this]() { return now; }, [this]() { return over; } };
	}

	// Move the clock, and let the effect see it
	void at( Scene &scene, double seconds )
	{
		now = seconds;
		if ( !scene.done() )
		{
			scene.resume();
		}
	}
};
}

TEST_CASE( "WordContext knows where its word is", "[words]" )
{
	Fixture f;

	const AABB first = f.word( 0, 0, 1 ).bounds();
	const AABB second = f.word( 1, 1, 2 ).bounds();
	const AABB third = f.word( 2, 2, 3 ).bounds();

	CHECK_FALSE( first.empty() );
	CHECK( first.width() > 0.F );
	// left to right, not overlapping
	CHECK( second.min.x() > first.max.x() );
	CHECK( third.min.x() > second.max.x() );

	const Point centre = f.word( 1, 1, 2 ).center();
	CHECK( centre.x() > second.min.x() );
	CHECK( centre.x() < second.max.x() );
}

TEST_CASE( "WordContext bounds follow the groups the word is in", "[words]" )
{
	Fixture f;
	const AABB before = f.word( 1, 0, 1 ).bounds();

	// moving the line the word is on (as centring or alignment do) moves the word as seen from the text
	auto &line = static_cast<Group &>( **f.text->begin() );
	line.position = { 3.F, 0.F };
	const AABB after = f.word( 1, 0, 1 ).bounds();

	CHECK( after.min.x() == Approx( before.min.x() + 3.F ) );
	CHECK( after.width() == Approx( before.width() ) );
}

TEST_CASE( "track_span reports progress from 0 to 1 and ends with exactly 1", "[words]" )
{
	double now = 0.0;
	bool over = false;
	std::vector<float> seen;
	Scene scene = track_span( [&]() { return now; }, [&]() { return over; }, 1.0, 3.0, [&]( float t ) { seen.push_back( t ); } );

	now = 0.0;
	scene.resume();
	CHECK( seen.empty() ); // not there yet
	now = 0.5;
	scene.resume();
	CHECK( seen.empty() );

	now = 1.0;
	scene.resume();
	now = 2.0;
	scene.resume();
	REQUIRE( seen.size() == 2 );
	CHECK( seen[0] == Approx( 0.F ) );
	CHECK( seen[1] == Approx( 0.5F ) );

	now = 3.0;
	scene.resume();
	CHECK( scene.done() );
	CHECK( seen.back() == 1.F );
}

TEST_CASE( "track_span finishes if the narration runs out", "[words]" )
{
	double now = 0.0;
	bool over = false;
	float last = -1.F;
	Scene scene = track_span( [&]() { return now; }, [&]() { return over; }, 5.0, 6.0, [&]( float t ) { last = t; } );

	scene.resume();
	over = true;
	scene.resume();
	CHECK( scene.done() );
	CHECK( last == 1.F );
}

TEST_CASE( "reveal hides the word until it is said, then fades it in", "[words]" )
{
	Fixture f;
	Scene effect = word_effects::reveal()( f.word( 1, 1.0, 2.0 ) );

	f.at( effect, 0.0 );
	CHECK( f.word( 1, 1, 2 ).word.color.a == 0.F ); // hidden as soon as it starts, before its word
	f.at( effect, 0.9 );
	CHECK( f.word( 1, 1, 2 ).word.color.a == 0.F );
	f.at( effect, 1.5 );
	CHECK( f.word( 1, 1, 2 ).word.color.a == Approx( 0.5F ) );
	f.at( effect, 2.0 );
	CHECK( f.word( 1, 1, 2 ).word.color.a == 1.F );
	CHECK( effect.done() );

	// other words are not touched
	CHECK( f.word( 0, 0, 1 ).word.color.a == 1.F );
}

TEST_CASE( "color takes a word from before to during to after", "[words]" )
{
	Fixture f;
	const Color grey{ 0.4F, 0.4F, 0.4F, 1.F };
	const Color yellow{ 1.F, 0.8F, 0.F, 1.F };
	const Color white{ 1.F, 1.F, 1.F, 1.F };
	Scene effect = word_effects::color( grey, yellow, white )( f.word( 0, 1.0, 2.0 ) );
	Visual &word = f.word( 0, 1, 2 ).word;

	f.at( effect, 0.0 );
	CHECK( word.color.r == Approx( 0.4F ) );

	f.at( effect, 1.0 );
	CHECK( word.color.r == Approx( 0.4F ) ); // just starting

	f.at( effect, 1.1 );
	// half way to yellow: a fifth of the way through the word is all it takes
	CHECK( word.color.r == Approx( 0.4F + ( 1.F - 0.4F ) * 0.5F ) );

	f.at( effect, 1.5 );
	CHECK( word.color.g == Approx( 0.8F ) ); // lit while it is said
	CHECK( word.color.b == Approx( 0.F ) );

	f.at( effect, 2.0 );
	CHECK( word.color.g == Approx( 0.8F ) );
	CHECK_FALSE( effect.done() ); // still settling

	f.at( effect, 2.125 );
	CHECK( word.color.b == Approx( 0.5F ) ); // half way to white

	f.at( effect, 2.5 );
	CHECK( word.color.g == Approx( 1.F ) );
	CHECK( word.color.b == Approx( 1.F ) );
	CHECK( effect.done() );

	SECTION( "and leaves the alpha alone" )
	{
		Fixture g;
		g.word( 0, 1, 2 ).word.color.a = 0.3F;
		Scene e = word_effects::color( grey, yellow, white )( g.word( 0, 1.0, 2.0 ) );
		g.at( e, 1.5 );
		CHECK( g.word( 0, 1, 2 ).word.color.a == Approx( 0.3F ) );
	}
}

TEST_CASE( "pop grows the word about its own centre and puts it back", "[words]" )
{
	Fixture f;
	Scene effect = word_effects::pop( 1.5F )( f.word( 1, 1.0, 2.0 ) );
	WordContext ctx = f.word( 1, 1.0, 2.0 );

	const AABB start = ctx.bounds();
	const Point centre = { ( start.min.x() + start.max.x() ) / 2.F, ( start.min.y() + start.max.y() ) / 2.F };

	f.at( effect, 0.0 );
	CHECK( ctx.word.scale_x == 1.F );

	f.at( effect, 1.5 ); // half way: the peak
	CHECK( ctx.word.scale_x == Approx( 1.5F ) );
	CHECK( ctx.word.scale_y == Approx( 1.5F ) );

	const AABB peak = ctx.bounds();
	CHECK( peak.width() == Approx( start.width() * 1.5F ) );
	// it grew about its centre: the middle did not move
	CHECK( ( peak.min.x() + peak.max.x() ) / 2.F == Approx( centre.x() ).margin( 1e-3 ) );
	CHECK( ( peak.min.y() + peak.max.y() ) / 2.F == Approx( centre.y() ).margin( 1e-3 ) );

	f.at( effect, 2.0 );
	CHECK( effect.done() );
	CHECK( ctx.word.scale_x == 1.F );
	CHECK( ctx.word.position.x() == Approx( 0.F ).margin( 1e-6 ) );
	CHECK( ctx.bounds().min.x() == Approx( start.min.x() ).margin( 1e-4 ) );
}

TEST_CASE( "underline draws a bar under the word as it is said", "[words]" )
{
	Fixture f;
	const size_t before = f.text->size();
	const AABB box = f.word( 1, 1, 2 ).bounds();
	Scene effect = word_effects::underline( Color{ 1.F, 0.F, 0.F, 1.F }, 0.1F, 0.05F )( f.word( 1, 1.0, 2.0 ) );

	f.at( effect, 0.0 );
	REQUIRE( f.text->size() == before + 1 );
	const auto &bar = dynamic_cast<const Rectangle &>( **( f.text->begin() + static_cast<std::ptrdiff_t>( before ) ) );
	CHECK( bar.half_width == 0.F );

	f.at( effect, 1.5 );
	CHECK( bar.half_width == Approx( box.width() / 4.F ) ); // half of the word's width, as a half width
	// it grows to the right from the start of the word, and sits below the letters
	CHECK( bar.position.x() - bar.half_width == Approx( box.min.x() ) );
	CHECK( bar.position.y() < box.min.y() );
	CHECK( bar.half_height == Approx( 0.05F ) );
	CHECK( bar.color == Color{ 1.F, 0.F, 0.F, 1.F } );

	f.at( effect, 2.0 );
	CHECK( effect.done() );
	CHECK( f.text->size() == before ); // taken away with the word

	SECTION( "unless it is to stay" )
	{
		Fixture g;
		const size_t n = g.text->size();
		Scene keep = word_effects::underline( Color{}, 0.1F, 0.05F, true )( g.word( 1, 1.0, 2.0 ) );
		g.at( keep, 0.0 );
		g.at( keep, 2.0 );
		CHECK( keep.done() );
		REQUIRE( g.text->size() == n + 1 );
		const auto &done = dynamic_cast<const Rectangle &>( **( g.text->begin() + static_cast<std::ptrdiff_t>( n ) ) );
		CHECK( done.half_width == Approx( box.width() / 2.F ) );
	}
}

TEST_CASE( "all runs several effects on the same word together", "[words]" )
{
	Fixture f;
	using namespace word_effects;
	Scene effect = all( { reveal(), pop( 1.2F ), color( Color{ 0.F, 0.F, 0.F, 1.F }, Color{ 1.F, 0.F, 0.F, 1.F }, Color{ 1.F, 1.F, 1.F, 1.F } ) } )( f.word( 2, 1.0, 2.0 ) );
	Visual &word = f.word( 2, 1, 2 ).word;

	f.at( effect, 0.0 );
	CHECK( word.color.a == 0.F ); // reveal's setup, at once
	CHECK( word.color.r == 0.F ); // color's setup, at once

	f.at( effect, 1.5 );
	CHECK( word.color.a == Approx( 0.5F ) );
	CHECK( word.color.r == Approx( 1.F ) );
	CHECK( word.scale_x == Approx( 1.2F ) );

	f.at( effect, 2.0 );
	f.at( effect, 2.5 );
	CHECK( effect.done() );
	CHECK( word.color.a == 1.F );
	CHECK( word.color.b == Approx( 1.F ) );
	CHECK( word.scale_x == 1.F );
}

TEST_CASE( "effects on different words are independent", "[words]" )
{
	Fixture f;
	Scene one = word_effects::reveal()( f.word( 0, 0.0, 1.0 ) );
	Scene two = word_effects::reveal()( f.word( 1, 1.0, 2.0 ) );

	f.at( one, 0.0 );
	f.at( two, 0.0 );
	CHECK( f.word( 0, 0, 1 ).word.color.a == 0.F );
	CHECK( f.word( 1, 1, 2 ).word.color.a == 0.F );

	f.at( one, 1.0 );
	f.at( two, 1.0 );
	CHECK( f.word( 0, 0, 1 ).word.color.a == 1.F ); // the first is fully in
	CHECK( f.word( 1, 1, 2 ).word.color.a == 0.F ); // the second is only starting
}
