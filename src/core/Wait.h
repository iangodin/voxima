
#pragma once

#include "Coroutine.h"

inline Coroutine wait_for( int64_t frames )
{
	while ( frames > 0 )
	{
		co_yield NextFrame{};
		--frames;
	}
}

inline Coroutine next_frame()
{
	co_yield NextFrame{};
}
