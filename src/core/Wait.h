
#pragma once

#include "Animation.h"

inline Animation wait_for( int64_t frames )
{
	while ( frames > 0 )
	{
		co_yield NextFrame{};
		--frames;
	}
}

