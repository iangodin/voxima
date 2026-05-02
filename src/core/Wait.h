
#pragma once

#include "Director.h"

inline Director wait_for( int64_t frames )
{
	while ( frames > 0 )
	{
		co_yield {};
		--frames;
	}
}

