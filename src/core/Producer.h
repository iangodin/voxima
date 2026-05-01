
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
		m_main_narrative(std::move(screenplay)),
		m_stage(stage)
	{
		m_main_narrative.handle.promise().producer_ptr = this;
	}

	int64_t current_frame( void ) const
	{
		return m_current_frame;
	}

	// This is called once per frame by your main loop (SDL3/Vulkan)
	bool tick()
	{
		++m_current_frame;

		if ( !m_main_narrative.done() )
		{
			m_main_narrative.resume();
		}

		return !m_main_narrative.done();
	}

private:
	Screenplay m_main_narrative;
	Stage &m_stage;
	int64_t m_current_frame = -1;
};

