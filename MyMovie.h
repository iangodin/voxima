
#include "Production.h"
#include "Stage.h"
#include "Scene.h"
#include "Producer.h"
#include "Screenplay.h"
#include "Director.h"

#include <coroutine>

Director background_rain( Stage &stage, int64_t frames )
{
	while ( frames > 0 )
	{
		stage.message += ".";
		--frames;
		co_yield {};
		}
}

Director background_snow( Stage &stage, int64_t frames )
{
	while ( frames > 0 )
	{
		stage.message += "o";
		--frames;
		co_yield {};
		}
}

Scene my_introduction( Stage &stage )
{
	stage.message = "My introduction";
	co_await wait_for( 5 );

	co_return;
}

Scene my_show( Stage &stage )
{
	stage.message = "My show";
	co_await wait_for( 5 );

    co_return;
}

Scene my_ending( Stage &stage )
{
	stage.message = "My ending";
	co_await together(
		background_rain( stage, 3 ),
		background_snow( stage, 1 ),
		wait_for( 5 )
	);

	stage.message = "Plot twist!";

    co_return;
}

Scene my_credits( Stage &stage )
{
	stage.message = "Credits rolling";
	co_await wait_for( 5 );

    co_return;
}

Screenplay my_movie( Stage &stage )
{
	co_await my_introduction( stage );
	co_await my_show( stage );
	co_await my_ending( stage );
	co_await my_credits( stage );
};
