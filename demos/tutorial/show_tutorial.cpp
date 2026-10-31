// The tutorial video's scenes, one per chapter/scene in outline.md. With no --scene the "movie" scene
// renders all of them in order; any one can be rendered on its own:
//
//     voxima --plugin build/Release/tutorial --script-dir demos/tutorial --scene what-is-voxima
//     voxima --plugin build/Release/tutorial --script-dir demos/tutorial --list-scenes
//
// See outline.md for the chapter/scene breakdown this follows.
//
// Narrated text (captions, and anything else timed to what is being said) is synced by line
// (narration.until_line(), at a line's start), not highlighted word by word: a line's start stays lined up
// across languages (the narration mixer divides at markers and at the start of each line - see
// doc/scenes.md), and stays correct even when a line's shown text and its spoken text differ in word count
// (a <written|spoken> substitution - see utils/AltText.h). A per-word effect has neither guarantee.

#include <core/Animate.h>
#include <core/Coroutine.h>
#include <core/Director.h>
#include <core/EaseCubic.h>
#include <core/MorphAnim.h>
#include <core/Narration.h>
#include <core/Plugin.h>
#include <core/Scene.h>
#include <core/SceneRegistry.h>
#include <core/Screenplay.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>

#include <utils/AltText.h>
#include <utils/Markers.h>

#include <visuals/Circle.h>
#include <visuals/CreateList.h>
#include <visuals/CreateText.h>
#include <visuals/Panel.h>
#include <visuals/Placement.h>
#include <visuals/Rectangle.h>
#include <visuals/WrapText.h>

using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{

// A sentence's worth of prose comfortably fits the 16-unit-wide stage (Camera::BASE_VIEW_WIDTH) at this
// width, wrapping instead of running off the edges however long the script's text turns out to be.
constexpr float CAPTION_WIDTH = 12.F;

// A caption at the bottom of the screen, in the place a real subtitle would be: one line, replaced (not
// stacked) as the narration reaches the next. `lines` is the script's raw text (markers and any
// <written|spoken> substitutions still in it - see utils/AltText.h); what is shown is always the written
// side, whether or not it was also spoken.
Scene show_subtitles( Stage &stage, Narration &narration, const std::vector<std::string> &lines, const std::string &lang )
{
	Visual *previous = nullptr;
	for ( size_t i = 0; i < lines.size(); ++i )
	{
		co_await narration.until_line( i );

		const std::string clean = strip_markers( written_text( lines[i] ) );
		auto &text = stage.visuals.add( make_paragraph(
			*stage.theme, clean, FontStyle{ Intent::PROSE }.large(), CAPTION_WIDTH, lang, TextLayout{ .align = TextAlign::CENTER } ) );
		move_to( text, { 0.F, -2.5F }, Anchor::BOTTOM );

		if ( previous != nullptr )
		{
			co_await choreograph( fade_out( stage, *previous, 0.2s ), fade_in( stage, text, 0.2s ) );
		}
		else
		{
			co_await fade_in( stage, text, 0.3s );
		}

		previous = &text;
	}
}

// Grows `visual` in once the narration reaches line `line_index`: the same idea as show_subtitles, for a
// visual that is not a narrated line of text (a panel, a shape, ...).
Scene reveal_at_line( Stage &stage, Narration &narration, size_t line_index, Visual &visual, std::chrono::duration<double> duration = 0.5s )
{
	co_await narration.until_line( line_index );
	co_await grow_in( stage, visual, duration );
}

std::vector<std::string> string_list( const huml::Value &value )
{
	std::vector<std::string> items;
	for ( const auto &item: value.as_list() )
	{
		items.push_back( std::string( item.as_string() ) );
	}
	return items;
}

// A circle becomes a square, then the letter Voxima starts with, then the same greeting in a few other
// languages: vector shapes, morphed by a coroutine, in more than one language - shown rather than narrated
// about. Paced to the same lines as show_subtitles (until_line), not a fixed clock.
Scene shape_story( Stage &stage, Narration &narration )
{
	const Point home = { 0.F, 0.1F };
	const auto series = stage.theme->palette().series();
	auto accent = [&series]( size_t i ) { return series[i % series.size()]; };

	auto label_at = [&]( const std::string &s, const Color &color )
	{
		auto text = make_text( *stage.theme, s, FontStyle{ Intent::PROSE }.huge() );
		center_group( *text );
		text->position = home;
		text->color = color;
		return text;
	};

	co_await narration.until_line( 1 ); // "Vector shapes, animated by coroutines."
	auto circle = std::make_unique<Circle>( 0.7F, accent( 0 ) );
	circle->position = home;
	Visual *shown = &stage.visuals.add( std::move( circle ) );
	co_await grow_in( stage, *shown, 0.4s );

	co_await narration.until_line( 2 ); // "Narrated, and subtitled, as it plays."
	{
		auto square = std::make_unique<Rectangle>( 0.6F, 0.6F, accent( 1 ) );
		square->position = home;
		Visual *next = square.get();
		co_await morph( stage, *shown, std::move( square ), 0.6s, ease_cubic_in_out );
		shown = next;
	}

	co_await narration.until_line( 3 ); // "In any language you choose."
	const std::vector<std::pair<std::string, size_t>> words = {
		{ "Bonjour", 2 }, { "Hallo", 3 }, { "¡Hola!", 4 } };
	for ( const auto &[word, color]: words )
	{
		auto next = label_at( word, accent( color ) );
		Visual *next_ptr = next.get();
		co_await morph( stage, *shown, std::move( next ), 0.4s, ease_cubic_in_out );
		shown = next_ptr;
		co_await wait_for( stage, 0.3s );
	}
}

}

// Chapter 1, Scene 1: the title, and a shape that shows what Voxima does while it is narrated - vector
// graphics, morphed by coroutines, subtitled, in whichever language is playing
Scene what_is_voxima( Stage &stage, Script &script )
{
	const std::string lang = script.master_language();

	Narration narration;
	narration.setup( "narrator", script, stage.audio );
	narration.speech( "what-is-voxima", "intro" );

	auto &title = stage.visuals.add( make_text( *stage.theme, script.label( lang, "title" ), FontStyle{ Intent::TITLE }.huge(), lang ) );
	move_to( title, { 0.F, 3.6F }, Anchor::TOP );

	const auto lines = script.fetch( lang, "what-is-voxima", "intro" ).as_multiline();

	co_await choreograph(
		grow_in( stage, title, 0.6s ),
		narration.play(),
		show_subtitles( stage, narration, lines, lang ),
		shape_story( stage, narration ) );
}

// Chapter 1, Scene 2: how to build the project, and where things live
Scene building_the_project( Stage &stage, Script &script )
{
	const std::string lang = script.master_language();

	Narration narration;
	narration.setup( "narrator", script, stage.audio );
	narration.speech( "building-the-project", "intro" );

	auto &title = stage.visuals.add( make_text( *stage.theme, script.label( lang, "building-title" ), FontStyle{ Intent::TITLE }.huge(), lang ) );
	center_group( title );
	title.position = { 0.F, 2.2F };

	Theme &theme = *stage.theme;
	const auto build_items = string_list( script.fetch( lang, "building-the-project", "build_steps" ) );
	const auto layout_items = string_list( script.fetch( lang, "building-the-project", "layout" ) );

	auto &build_panel = stage.visuals.add( make_panel( make_list( theme, build_items ) ) );
	auto &layout_panel = stage.visuals.add( make_panel( make_list( theme, layout_items ) ) );
	move_to( build_panel, { -2.4F, -0.3F }, Anchor::CENTER );
	next_to( layout_panel, build_panel, Anchor::RIGHT, 1.0F );

	co_await choreograph(
		grow_in( stage, title, 0.6s ),
		narration.play(),
		grow_in( stage, build_panel, 0.5s ),
		reveal_at_line( stage, narration, 2, layout_panel ) ); // the line about where things live
}

Scene movie( Stage &stage, Script &script )
{
	co_await sequence( stage, script, { "what-is-voxima", "building-the-project" } );
}

REGISTER_SCENE( "what-is-voxima", "Title card and narrated line: what Voxima is and what it's for", what_is_voxima );
REGISTER_SCENE( "building-the-project", "Build commands and project layout, in two panels", building_the_project );
REGISTER_SCENE( "movie", "The whole tutorial, in order", movie );
