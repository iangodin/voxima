#pragma once

#include "Coroutine.h"

#include <chrono>

struct Stage;

// Pause the scene for a length of time (0.5s, 250ms ...), at the stage's frame rate (Camera::frames). A number
// without a unit does not compile: it would be seconds or frames, and a scene should not have to guess.
Coroutine wait_for( const Stage &stage, std::chrono::duration<double> duration );

// Pause for exactly this many frames, whatever the frame rate: for something that really is about frames
Coroutine wait_frames( int64_t frames );

inline Coroutine next_frame()
{
	co_yield NextFrame{};
}
