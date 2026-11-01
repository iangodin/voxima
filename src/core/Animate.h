#pragma once

#include "core/Ease.h"
#include "core/Scene.h"
#include "core/Stage.h"
#include "visuals/Group.h"
#include "visuals/Path.h"
#include "visuals/Placement.h"
#include "visuals/Point.h"
#include "visuals/Visual.h"

#include <chrono>
#include <functional>
#include <vector>
#include <optional>

// The common things to do to a visual, each a scene to `co_await` (so it is over when they are, or run several at once with
// choreograph). Each takes the visual as it is now as its resting state and leaves it there, except the ones that take it
// away. The visual has to stay alive as long as the animation runs. All take the length in seconds and an ease.
//
//     auto &box = stage.visuals.add( make_panel( ... ) );
//     co_await fade_in( stage, box, 0.5s );
//     co_await indicate( stage, box );                       // a pulse, to draw the eye
//     co_await slide_out( stage, box, { 4.F, 0.F }, 0.6s );  // away to the right, fading
//
// Fading changes the alpha of the visual's own color and stroke color (a group's colors multiply down into its children, so
// a group fades as one).

// Fade from nothing to how it is; fade to nothing (it is left there, invisible: remove it if it should not be kept)
Scene fade_in( const Stage &stage, Visual &visual, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );
Scene fade_out( const Stage &stage, Visual &visual, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// Fade to an opacity and stay there: to dim what is not the point, and then bring it back with an opacity of 1. It sets the visual's
// opacity (Visual::opacity), which fade_in and fade_out leave alone, so a group fades as one, without what overlaps in it
// showing through.
//
//     co_await fade_to( stage, rest_of_it, 0.3F );           // focus on this ...
//     co_await fade_to( stage, rest_of_it, 1.F );            // ... and back
Scene fade_to( const Stage &stage, Visual &visual, float opacity, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// Grow from a point (its own center) to full size, fading in with it; and the reverse
Scene grow_in( const Stage &stage, Visual &visual, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );
Scene shrink_out( const Stage &stage, Visual &visual, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// Grow from one of its edges to full size, that edge staying where it is: BOTTOM for a bar of a chart, LEFT for a gauge (a corner
// grows both ways, and CENTER is grow_in with no fade). For what is not turned.
Scene grow_from_edge( const Stage &stage, Visual &visual, Anchor edge, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// Come in from `offset` away (stage units) to where it is, fading in; and go out to `offset` away, fading out
Scene slide_in( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );
Scene slide_out( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// Move to `to` (stage units, in its parent's frame)
Scene move_by( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// A pulse to draw the eye: bigger by `factor` at its most, about its own center, and back
Scene indicate( const Stage &stage, Visual &visual, float factor = 1.2F, std::chrono::duration<double> duration = std::chrono::milliseconds( 600 ) );

// Draw a visual on: its outlines are traced by a line, each as far along as the others (for text, the letters' shapes), and
// then the visual itself fades in over them. The first `trace` (0 to 1, of the time) is the tracing, the rest the fade.
// It is the visual as a whole that comes in, so it can be a shape, a path, a piece of text or a group of them.
// The outlines are drawn in `parent` (the group the visual is a child of; the stage's visuals if it is not given), which is
// also where the visual has to be a direct child of, so that they line up.
Scene draw_on( Stage &stage, Visual &visual, std::chrono::duration<double> duration = std::chrono::milliseconds( 1500 ), float trace = 0.7F, Group *parent = nullptr );

struct PointOutOptions
{
	// Room between the visual's box and the line round it (stage units), and how rounded its corners are
	float padding = 0.2F;
	float radius = 0.2F;
	float thickness = 0.06F;

	// Default: the palette's yellow
	std::optional<Color> color = {};
};

// Draw a line round a visual, as if with a pen, and let it fade; the visual is left as it was. `parent` is as for draw_on.
Scene circumscribe( Stage &stage, Visual &visual, const PointOutOptions &options = {}, std::chrono::duration<double> duration = std::chrono::milliseconds( 1200 ), Group *parent = nullptr );

// A ring round a visual that spreads out from its edge and fades: a flash to say "here"
Scene flash( Stage &stage, Visual &visual, const PointOutOptions &options = {}, std::chrono::duration<double> duration = std::chrono::milliseconds( 600 ), Group *parent = nullptr );

// Move along a path (its points in the visual's parent's frame), from its start to its end, the visual's own origin on it.
// With `turn` the visual is turned to face the way it is going (its own +x forward), else it keeps its rotation.
Scene move_along( const Stage &stage, Visual &visual, const Path &path, std::chrono::duration<double> duration = std::chrono::milliseconds( 1000 ), EaseFunction ease = {}, bool turn = false );

// Turn about a point (in the visual's parent's frame) by `degrees`, counterclockwise on the screen if positive: the visual
// goes round the point and turns with it
Scene turn_about( const Stage &stage, Visual &visual, Point center, float degrees, std::chrono::duration<double> duration = std::chrono::milliseconds( 1000 ), EaseFunction ease = {} );

// Run a verb on each of the visuals, each starting `delay` after the one before, and finish when the last does:
//
//     co_await stagger( stage, children_of( row ), 0.15s, [&]( Visual &v ) { return fade_in( stage, v, 0.4s ); } );
Scene stagger( const Stage &stage, std::vector<Visual *> visuals, std::chrono::duration<double> delay, std::function<Scene( Visual & )> verb );

// The direct children of a group, to stagger over
std::vector<Visual *> children_of( const Group &group );
