
#pragma once

#include "Coroutine.h"
#include "Production.h"

#include <coroutine>
#include <vector>
#include <memory>

using Director = Coroutine;

template <typename ...Coroutines>
bool advance( Coroutines &...coros )
{
	// Resume every coroutines in the pack
	( ..., ( !coros.done() ? coros.resume() : void() ) );

	// Return true if at least one animation is still running
	return ( ... || ( !coros.done() ) );
}

template <typename ...Coroutines>
Director choreograph( Coroutines ...coros )
{
	while ( advance( coros... ) )
	{
		co_yield {};
	}

	co_return;
}
