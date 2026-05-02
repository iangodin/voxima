
#pragma once

#include "core/Director.h"
#include "core/Screenplay.h"
#include "core/Stage.h"

#include <cstdint>
#include <vector>

class Producer
{
public:
	// We give the Producer the top-level Screenplay to run
	Producer( Screenplay &&screenplay, Stage &stage) :
		_main_narrative(std::move(screenplay)),
		_stage(stage)
	{
		_main_narrative.handle.promise().producer_ptr = this;
	}

	int64_t current_frame( void ) const
	{
		return _current_frame;
	}

	bool tick()
	{
		++_current_frame;

		if ( !_main_narrative.done() )
		{
			_main_narrative.resume();
		}

		return !_main_narrative.done();
	}

private:
	Screenplay _main_narrative;
	Stage &_stage;
	int64_t _current_frame = -1;
};

