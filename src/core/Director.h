
#pragma once

#include "Coroutine.h"
#include "Production.h"

#include <functional>
#include <coroutine>
#include <vector>
#include <memory>

struct Stage;
class Script;

using Director = std::function<Coroutine( Stage &, Script & )>;

template <typename ...Coroutines>
bool advance( Coroutines &...coros )
{
	// Resume every coroutines in the pack
	( ..., ( !coros.done() ? coros.resume() : void() ) );

	// Return true if at least one animation is still running
	return ( ... || ( !coros.done() ) );
}

template <typename ...Coroutines>
Coroutine choreograph( Coroutines &&...coros )
{
	while ( advance( coros... ) )
	{
		co_yield {};
	}

	co_return;
}
