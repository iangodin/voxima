// Coroutines: a scene that waits, things that run at the same time, something that changes every frame, and a label
// that follows a moving dot.
//
// Build it (see CMakeLists.txt), then
//
//     voxima --plugin build/coroutines --script-dir scripts --lang en --scene coroutine-basics --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <core/Animate.h>
#include <core/Coroutine.h>
#include <core/EaseCubic.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Tween.h>
#include <core/Wait.h>
#include <visuals/Circle.h>
#include <visuals/CreateText.h>
#include <visuals/Path.h>
#include <visuals/Placement.h>

#include <chrono>
#include <memory>
#include <vector>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
// A scene is a coroutine: it runs until a co_await, and carries on from there on a later frame
// --8<-- [start:intro]
Scene intro( Stage &stage, Script & )
{
	auto &title = stage.visuals.add( make_text( *stage.theme, "Hello", FontStyle{ Intent::TITLE } ) );
	co_await fade_in( stage, title, 0.5s );   // frames pass here
	co_await wait_for( stage, 1s );
	co_await fade_out( stage, title, 0.5s );
}
// --8<-- [end:intro]

Scene show_coroutines( Stage &stage, Script &script )
{
	co_await intro( stage, script );
	stage.visuals.clear();

	auto &title = stage.visuals.add( make_text( *stage.theme, "Together", FontStyle{ Intent::TITLE } ) );
	move_to( title, { 0.F, 3.5F }, Anchor::TOP );
	auto &circle = stage.visuals.add( std::make_unique<Circle>( 0.6F, stage.theme->palette().blue ) );
	circle.position = { -4.F, 0.F };

	// --8<-- [start:pauses]
	co_await wait_for( stage, 1.5s );     // a pause
	co_await wait_frames( 10 );           // exactly 10 frames, whatever the frame rate
	co_await next_frame();                // one frame
	// --8<-- [end:pauses]

	// --8<-- [start:choreograph]
	co_await choreograph(
		fade_in( stage, title, 0.5s ),
		grow_in( stage, circle, 0.8s ),
		wait_for( stage, 1s ) );              // (a narration.play() goes here, to run with them)
	// --8<-- [end:choreograph]

	std::vector<Visual *> items;
	for ( int i = 0; i < 4; ++i )
	{
		auto &item = stage.visuals.add( std::make_unique<Circle>( 0.3F, stage.theme->palette().green ) );
		item.position = { -3.F + 2.F * static_cast<float>( i ), -2.5F };
		items.push_back( &item );
	}
	// --8<-- [start:choreograph-all]
	std::vector<Coroutine> running;
	for ( Visual *item: items )
		running.push_back( fade_in( stage, *item, 0.4s ) );
	co_await choreograph_all( std::move( running ) );
	// --8<-- [end:choreograph-all]

	// --8<-- [start:tween]
	co_await tween( stage, 1.5s, ease_cubic_in_out, [&]( float t )
	{
		circle.position = { -4.F + 8.F * t, 0.F };
	} );
	// --8<-- [end:tween]

	auto &label = stage.visuals.add( make_text( *stage.theme, "dot", FontStyle{ Intent::LABEL } ) );
	auto &dot = circle;
	Path path;
	path.commands = { MoveTo{ { 4.F, 0.F } }, CubicTo{ { 2.F, 2.F }, { -2.F, 2.F }, { -4.F, 0.F } } };
	// --8<-- [start:updater]
	auto tie = stage.updaters.add( [&] { label.position = dot.position + Point{ 0.F, 0.4F }; } );
	co_await move_along( stage, dot, path, 2s );     // the label goes with the dot
	// --8<-- [end:updater]
	co_await wait_for( stage, 1s );
}
}

REGISTER_SCENE( "coroutine-basics", "Waiting, doing things together, every-frame changes, and a label that follows", show_coroutines );
