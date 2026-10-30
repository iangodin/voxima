#pragma once

#include "Scene.h"
#include "Stage.h"
#include "Wait.h"

#include <algorithm>
#include <chrono>
#include <cstdint>

// Run something over a length of time (1.5s ...): calls `callback( ease( t ) )` once a frame, with t going from 0 on
// the first frame to 1 on the last, then finishes. The time is turned into whole frames at the stage's frame rate
// (see Camera::frames), and is never less than two frames (a first and a last).
//
//     co_await tween( stage, 1.5s, ease_cubic_in_out, [&]( float t ) { circle.position = { x1 + ( x2 - x1 ) * t, y }; } );
//
// `ease` is any function from t to t (see Ease.h): ease_cubic_out, ease_back_in_out, ease_linear, or your own.
// The last value given is ease( 1 ), which is 1 for the eases here.
template<typename Ease, typename Callback>
Scene tween( const Stage &stage, std::chrono::duration<double> duration, Ease &&ease, Callback &&callback )
{
	const int64_t frames = std::max<int64_t>( 2, stage.camera.frames( duration ) );
	for ( int64_t frame = 0; frame < frames; ++frame )
	{
		callback( ease( static_cast<float>( frame ) / static_cast<float>( frames - 1 ) ) );
		co_await next_frame();
	}
}
