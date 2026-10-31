// Narration: speech in a chosen voice, in every language of the script, with animation that follows the words (karaoke, and a shape that changes on markers).
//
// Build it (see CMakeLists.txt), then
//
//     voxima --plugin build/narration --script-dir scripts --lang en --list-scenes
//     voxima --plugin build/narration --script-dir scripts --lang en --scene eases --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <code/LineHighlight.h>
#include <core/Animate.h>
#include <core/EaseBack.h>
#include <core/EaseElastic.h>
#include <core/EaseLinear.h>
#include <core/MorphAnim.h>
#include <core/Narration.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>
#include <utils/Markers.h>
#include <visuals/CreateText.h>

#include <chrono>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
template<typename Ease>
Scene show_ease(
	Stage &stage, const std::string &name, const std::string &lang, Ease &&ease, float y, std::chrono::duration<double> duration, const Color &color )
{
	float x1 = -4.F;
	float x2 = 7.F;
	auto &circle = stage.visuals.add( std::make_unique<Circle>() );
	circle.radius = 0.1F;
	circle.color = color;
	circle.position = { x1, y };

	auto &label = stage.visuals.add( make_text( *stage.theme, name, FontStyle{ Intent::LABEL }, lang ) );
	label.position = { -6.F, y };

	co_await tween(
		stage, duration, ease, [&]( float t ) { circle.position = { x1 + ( x2 - x1 ) * t, circle.position.y() }; } );
}

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

// A shape that changes on the phrases being said: a circle turns into a square between the markers {a} and {b}
// in the script, into an a from {b} to {c}, and back into a circle from {c} to {d}. The markers are in the text of
// every language, so this follows the narration whichever language it is in (word numbers would not).
Scene follow_speech( Stage &stage, Narration &narration )
{
	const Point spot = { 5.5F, 2.5F };
	auto smooth = []( float t ) { return t * t * ( 3.F - 2.F * t ); };

	Visual *shown = &stage.visuals.add( demo_circle( stage, spot ) );

	auto square = demo_square( stage, spot );
	Visual *next = square.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( square ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_markers( "a", "b", [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
	shown = next;

	auto letter = demo_letter( stage, "a", stage.theme->palette().yellow, spot );
	next = letter.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( letter ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_markers( "b", "c", [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
	shown = next;

	auto circle = demo_circle( stage, spot );
	next = circle.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( circle ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_markers( "c", "d", [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
}

// Narrated text with an effect on every word as it is said. The first line is dim until its words are spoken:
// each lights up yellow, pops, and settles to the text color. The second line is invisible until its words are spoken:
// each fades in, flashes yellow, and is underlined as it is said. Beside them a shape changes on the words.
Scene show_karaoke( Stage &stage, Script &script )
{
	using namespace word_effects;

	const std::string lang = script.master_language();
	// The colors of the palette, so the words show on a light background as on a dark one: not yet said (muted), said
	// (the palette's yellow: a dark gold on a light palette), and said before (the text color itself: what the effect
	// blends to has to be a color, not "white", or a word passes through a pale gray on its way to dark text)
	const Palette &palette = stage.theme->palette();
	const Color dim = palette.muted;
	const Color white = stage.foreground;
	const Color yellow = palette.yellow;

	Narration narration;
	narration.setup( "narrator", script, stage.audio );
	narration.speech( "intro" );

	// One text per narrated line, shown as it is written: word N spoken is word N of the line, from the first
	// word of that line on.
	std::vector<Coroutine> running;
	running.push_back( narration.play() );
	running.push_back( follow_speech( stage, narration ) );

	size_t first_word = 0;
	size_t line_number = 0;
	const auto lines = script.fetch( lang, "intro" ).as_multiline();
	for ( const auto &line: lines )
	{
		const std::string clean = strip_markers( line ); // the markers are cue points, not text
		StyledText styled( clean );
		styled.set_font_style( 0, styled.size(), FontStyle{ Intent::PROSE }.large() );
		styled.set_language( 0, styled.size(), lang );

		auto &text = stage.visuals.add( make_styled_text( *stage.theme, styled ) );
		center_group( text );
		text.position = { 0.F, 1.F - 0.9F * static_cast<float>( line_number ) };

		const WordEffect effect = line_number % 2 == 0
									  ? all( { color( dim, yellow, white ), pop( 1.15F ) } )
									  : all( { reveal(), color( white, yellow, white ), underline( yellow ) } );
		running.push_back( narration.animate_words( text, effect, first_word ) );

		first_word += split_words( clean ).size();
		++line_number;
	}

	co_await choreograph_all( std::move( running ) );
}

// The ease functions side by side, as circles racing across the screen while the narrator speaks
Scene show_eases( Stage &stage, Script &script )
{
	const auto duration = 1s;

	const std::string not_found{ "\uFFFD" };
	const std::string lang = script.master_language();
	auto label = [&]( const char *key ) { return std::string( script.fetch( lang, "labels", key ).as_string( not_found ) ); };

	Narration narration;
	narration.setup( "narrator", script, stage.audio );
	narration.speech( "intro" );

	// Each curve in a color of the palette, in turn: the lanes can be told apart
	const auto series = stage.theme->palette().series();
	auto accent = [&series]( size_t i ) { return series[i % series.size()]; };

	co_await choreograph(
		show_ease( stage, label( "ease_cubic_in" ), lang, ease_cubic_in, 3.F, duration, accent( 0 ) ),
		show_ease( stage, label( "ease_cubic_out" ), lang, ease_cubic_out, 2.5F, duration, accent( 1 ) ),
		show_ease( stage, label( "ease_cubic_in_out" ), lang, ease_cubic_in_out, 2.0F, duration, accent( 2 ) ),
		show_ease( stage, label( "ease_elastic_in" ), lang, ease_elastic_in, 1.5F, duration, accent( 3 ) ),
		show_ease( stage, label( "ease_elastic_out" ), lang, ease_elastic_out, 1.0F, duration, accent( 4 ) ),
		show_ease( stage, label( "ease_elastic_in_out" ), lang, ease_elastic_in_out, 0.5F, duration, accent( 5 ) ),
		show_ease( stage, label( "ease_back_in" ), lang, ease_back_in, 0.F, duration, accent( 6 ) ),
		show_ease( stage, label( "ease_back_out" ), lang, ease_back_out, -0.5F, duration, accent( 7 ) ),
		show_ease( stage, label( "ease_back_in_out" ), lang, ease_back_in_out, -1.0F, duration, accent( 8 ) ),
		show_ease( stage, label( "ease_linear" ), lang, ease_linear, -1.5F, duration, accent( 9 ) ),
		narration.play() );
}
}

REGISTER_SCENE( "eases", "The ease functions, as circles racing across the screen (with narration)", show_eases );
REGISTER_SCENE( "karaoke", "Narrated text lit word by word, and a shape changing on the words", show_karaoke );
