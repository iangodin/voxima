
#pragma once

#include "core/Production.h"

#include <coroutine>
#include <memory>

struct Screenplay
{
	struct promise_type : public ProductionPromise
	{
		Screenplay get_return_object()
		{
			return Screenplay{std::coroutine_handle<promise_type>::from_promise(*this)};
		}

		// Start suspended so the Producer can decide when the "Action" starts
		std::suspend_always initial_suspend()
		{
			return {};
		}

		// Keep the state alive until the Producer explicitly destroys it
		std::suspend_always final_suspend() noexcept
		{
			return {};
		}

		void unhandled_exception()
		{
			std::terminate();
		}

		void return_void()
		{
		}

		std::suspend_always yield_value( NextFrame )
		{
            return {};
        }
	};

	std::coroutine_handle<promise_type> handle;

	Screenplay(std::coroutine_handle<promise_type> h) : handle(h) {}

	// Support move semantics, disable copying
	Screenplay(const Screenplay&) = delete;
	Screenplay(Screenplay&& other) noexcept : handle(other.handle) { other.handle = nullptr; }

	~Screenplay()
	{
		if (handle)
		{
			handle.destroy();
		}
	}

	bool done() const
	{
		return !handle || handle.done();
	}

	void resume()
	{
		if ( handle && !handle.done() )
		{
			if ( handle.promise().active_child_resume )
			{
				handle.promise().active_child_resume();
			}
			else
			{
				handle.resume();
			}
		}
	}
};
