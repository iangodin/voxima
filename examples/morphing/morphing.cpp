// Morphing: shapes, letters, formulas, sentences, strokes and pictures that turn into one another, with sound effects.
//
// Build it (see CMakeLists.txt), then
//
//     voxima --plugin build/morphing --script-dir scripts --lang en --list-scenes
//     voxima --plugin build/morphing --script-dir scripts --lang en --scene morph-shapes --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <code/LineHighlight.h>
#include <core/Animate.h>
#include <core/MorphAnim.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>
#include <math/Operators.h>
#include <visuals/CreateText.h>
#include <visuals/Layout.h>

#include <chrono>
#include <cmath>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
std::unique_ptr<Circle> demo_circle( const Stage &stage, Point at )
{
	const Palette &palette = stage.theme->palette();
	auto c = std::make_unique<Circle>();
	c->radius = 0.8F;
	c->position = at;
	c->color = palette.red;
	return c;
}

std::unique_ptr<Rectangle> demo_square( const Stage &stage, Point at )
{
	auto r = std::make_unique<Rectangle>();
	r->half_width = r->half_height = 0.7F;
	r->position = at;
	r->color = stage.theme->palette().blue;
	return r;
}

std::unique_ptr<Group> demo_letter( Stage &stage, const char *s, const Color &color, Point at )
{
	auto text = make_text( *stage.theme, s, FontStyle{ Intent::PROSE }.huge() );
	center_group( *text );
	text->position = at;
	text->color = color;
	return text;
}

// A plucked note, made rather than read from a file: a sine that dies away over `seconds`
AudioBuffer pluck( float frequency, float seconds = 2.F )
{
	constexpr size_t rate = 48000;
	AudioBuffer note( static_cast<size_t>( seconds * rate ), rate, 1 );
	for ( size_t f = 0; f < note.frames(); ++f )
	{
		const float t = static_cast<float>( f ) / rate;
		const float attack = std::min( 1.F, t / 0.005F ); // no click at the start
		note.frame( f )[0] = attack * std::exp( -3.F * t / seconds * 2.F ) * std::sin( 2.F * 3.14159265F * frequency * t );
	}
	return note;
}

// A shape that turns into other shapes and letters, one after the other: circle, square, then glyphs
// (with holes that open and close) and back to the circle. Each change sounds a two-note chord (sound effects
// play together, and ring on into the next).
Scene show_morph( Stage &stage, Script & )
{
	const Point home = { 0.F, 2.5F };
	auto circle = [&]() { return demo_circle( stage, home ); };
	auto square = [&]() { return demo_square( stage, home ); };
	auto letter = [&]( const char *s, const Color &color ) { return demo_letter( stage, s, color, home ); };

	Visual *shown = &stage.visuals.add( circle() );
	co_await wait_for( stage, 0.5s );

	std::vector<std::unique_ptr<Visual>> targets;
	targets.push_back( square() );
	targets.push_back( letter( "a", stage.theme->palette().yellow ) );
	targets.push_back( letter( "e", stage.theme->palette().green ) );
	targets.push_back( letter( "8", Color{ 1.F, 1.F, 1.F, 1.F } ) );
	targets.push_back( letter( "x", stage.theme->palette().blue ) );
	targets.push_back( circle() );

	const float notes[] = { 261.6F, 293.7F, 329.6F, 392.0F, 440.0F, 523.3F }; // C D E G A C
	size_t note = 0;
	for ( auto &target: targets )
	{
		Visual *next = target.get();
		const float root = notes[note++ % std::size( notes )];
		stage.audio.play( pluck( root ), { .volume = -14.0 } );
		stage.audio.play( pluck( root * 1.5F ), { .volume = -18.0 } ); // and a fifth above
		co_await morph( stage, *shown, std::move( target ), 1.5s, ease_cubic_in_out );
		shown = next;
		co_await wait_for( stage, 0.5s );
	}
}

// A formula that is rewritten step by step. Parts that stay the same (the a, the b, the plus signs) only
// slide into place, the rest morph or fade: leaves are matched by their tags, then by identical glyphs.
Scene show_formula_morph( Stage &stage, Script & )
{
	using namespace math;
	Context context( *stage.theme );

	auto build = [&]( const Expr &formula )
	{
		auto visual = formula.build( context );
		const AABB box = layout_extent( *visual );
		visual->position = { -( box.min.x() + box.max.x() ) / 2.F, -2.5F };
		return visual;
	};

	// Tagged parts are followed from one formula to the next wherever they end up; everything else is matched
	// by identical glyphs, and what has no counterpart fades (the parentheses, the new terms)
	const Expr a = tag( "a", text( "a" ) );
	const Expr b = tag( "b", text( "b" ) );
	const Expr expanded = row(
		{ ord( superscript( a, text( "2" ) ) ),
		  bin( "+" ),
		  ord( text( "2ab" ) ),
		  bin( "+" ),
		  ord( superscript( b, text( "2" ) ) ) } );

	Visual *shown = &stage.visuals.add( build( superscript( parens( row( { ord( a ), bin( "+" ), ord( b ) } ) ), text( "2" ) ) ) );
	co_await wait_for( stage, 0.5s );

	std::vector<std::unique_ptr<Visual>> steps;
	steps.push_back( build( expanded ) );
	steps.push_back( build( radical( expanded ) ) );

	for ( auto &step: steps )
	{
		Visual *next = step.get();
		co_await morph( stage, *shown, std::move( step ), 1.7s, ease_cubic_in_out );
		shown = next;
		co_await wait_for( stage, 0.5s );
	}
}

// A sentence that is edited: words that stay put only slide into place, words that go away fade out, new
// ones fade in, and a word that is replaced turns into its replacement letter by letter.
Scene show_text_morph( Stage &stage, Script & )
{
	auto line = [&]( const char *sentence )
	{
		auto text = make_text( *stage.theme, sentence, FontStyle{ Intent::PROSE }.large() );
		center_group( *text );
		text->position = { 0.F, -1.2F };
		return text;
	};

	Visual *shown = &stage.visuals.add( line( "The big cat sat down" ) );
	co_await wait_for( stage, 0.5s );

	std::vector<std::unique_ptr<Visual>> sentences;
	sentences.push_back( line( "The cat sat down" ) );        // "big" fades away
	sentences.push_back( line( "The cat sat on the mat" ) );  // "on the mat" fades in
	sentences.push_back( line( "The dog ran off" ) );         // words replaced, "The" stays

	for ( auto &sentence: sentences )
	{
		Visual *next = sentence.get();
		co_await morph( stage, *shown, std::move( sentence ), 1.7s, ease_cubic_in_out );
		shown = next;
		co_await wait_for( stage, 0.5s );
	}
}

// Strokes, arrows, letters that morph one after another, and pictures: a line inflates into a disc, which
// becomes an arrow that then turns; the arrow becomes a word whose letters change in a ripple; the word
// becomes a picture, which moves and fades into a disc.
Scene show_more_morphs( Stage &stage, Script & )
{
	const Point spot = { 0.F, 1.F };

	auto line = std::make_unique<Line>();
	line->start = { -2.F, 0.F };
	line->end = { 2.F, 0.F };
	line->stroke_thickness = 0.12F;
	line->stroke_color = stage.theme->palette().yellow;
	line->position = spot;

	auto arrow = [&]( float degrees )
	{
		auto group = std::make_unique<Group>();
		auto shaft = std::make_unique<Line>();
		shaft->start = { -1.5F, 0.F };
		shaft->end = { 1.2F, 0.F };
		shaft->stroke_thickness = 0.12F;
		shaft->stroke_color = stage.theme->palette().green;
		auto tip = std::make_unique<ArrowTip>();
		tip->start = { -1.5F, 0.F };
		tip->end = { 1.6F, 0.F };
		tip->size = 0.3F;
		tip->color = stage.theme->palette().green;
		group->add( std::move( shaft ) );
		group->add( std::move( tip ) );
		group->position = spot;
		group->rotation = degrees;
		return group;
	};
	auto word = [&]( const char *s )
	{
		auto text = make_text( *stage.theme, s, FontStyle{ Intent::PROSE }.huge() );
		center_group( *text );
		text->position = spot;
		return text;
	};
	auto picture = [&]( Point at, float scale )
	{
		auto image = std::make_unique<Image>();
		image->path = "assets/images/testcard.png";
		image->half_width = image->half_height = 1.2F;
		image->position = at;
		image->scale_x = image->scale_y = scale;
		return image;
	};

	struct Step
	{
		std::unique_ptr<Visual> to;
		std::chrono::duration<double> duration;
		MorphOptions options;
	};
	std::vector<Step> steps;
	steps.push_back( { demo_circle( stage, spot ), 1.7s, {} } );  // the line inflates into a disc
	steps.push_back( { arrow( 0.F ), 1.7s, {} } );         // the disc becomes an arrow, and
	steps.push_back( { arrow( 60.F ), 1.7s, {} } );        // the arrow turns
	MorphOptions ripple;
	ripple.stagger = 0.6F;
	ripple.part_ease = []( float t ) { return t * t * ( 3.F - 2.F * t ); };
	steps.push_back( { word( "smooth" ), 1.7s, {} } );     // the arrow becomes a word
	steps.push_back( { word( "morph" ), 2.5s, ripple } );  // whose letters change one after another
	steps.push_back( { picture( spot, 1.F ), 1.7s, {} } ); // the word cross-fades into a picture
	steps.push_back( { picture( { 3.F, spot.y() }, 0.6F ), 1.7s, {} } ); // which moves and shrinks
	steps.push_back( { demo_circle( stage, { 3.F, spot.y() } ), 1.7s, {} } );   // and fades into a disc

	Visual *shown = &stage.visuals.add( std::move( line ) );
	co_await wait_for( stage, 0.5s );
	for ( auto &step: steps )
	{
		Visual *next = step.to.get();
		co_await morph( stage, *shown, std::move( step.to ), step.duration, ease_cubic_in_out, step.options );
		shown = next;
		co_await wait_for( stage, 0.5s );
	}
}
}

REGISTER_SCENE( "morph-shapes", "A circle turning into a square and letters, with holes opening and closing", show_morph );
REGISTER_SCENE( "morph-formula", "(a+b)^2 rewritten step by step, matching parts by tag and glyph", show_formula_morph );
REGISTER_SCENE( "morph-text", "A sentence edited word by word", show_text_morph );
REGISTER_SCENE( "morph-strokes", "A line, an arrow, staggered letters and a picture morphing", show_more_morphs );
