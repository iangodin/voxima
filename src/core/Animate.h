#pragma once

#include "core/Ease.h"
#include "core/Scene.h"
#include "core/Stage.h"
#include "visuals/Point.h"
#include "visuals/Visual.h"

#include <chrono>

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

// Grow from a point (its own center) to full size, fading in with it; and the reverse
Scene grow_in( const Stage &stage, Visual &visual, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );
Scene shrink_out( const Stage &stage, Visual &visual, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// Come in from `offset` away (stage units) to where it is, fading in; and go out to `offset` away, fading out
Scene slide_in( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );
Scene slide_out( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// Move to `to` (stage units, in its parent's frame)
Scene move_by( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration = std::chrono::milliseconds( 500 ), EaseFunction ease = {} );

// A pulse to draw the eye: bigger by `factor` at its most, about its own center, and back
Scene indicate( const Stage &stage, Visual &visual, float factor = 1.2F, std::chrono::duration<double> duration = std::chrono::milliseconds( 600 ) );
