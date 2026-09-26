#include "Wait.h"

#include "Stage.h"

Coroutine wait_for( const Stage &stage, std::chrono::duration<double> duration )
{
	return wait_frames( stage.camera.frames( duration ) );
}

Coroutine wait_frames( int64_t frames )
{
	while ( frames > 0 )
	{
		co_yield NextFrame{};
		--frames;
	}
}
