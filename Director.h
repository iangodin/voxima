
#pragma once

#include "Production.h"

#include <coroutine>
#include <vector>
#include <memory>

struct Director
{
	struct promise_type : public ProductionPromise
	{
		Director get_return_object()
		{
			return Director{std::coroutine_handle<promise_type>::from_promise(*this)};
		}

		std::suspend_always initial_suspend()
		{
			return {};
		}

		ReturnToParent final_suspend() noexcept
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

	Director(std::coroutine_handle<promise_type> h) :
		handle(h)
	{
	}

	Director(Director&& other) noexcept : handle(other.handle) { other.handle = nullptr; }

	~Director()
	{
		if (handle)
		{
			handle.destroy();
		}
	}

	void resume()
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

	// --- THE AWAITER INTERFACE ---
	bool await_ready() const noexcept
	{
		return !handle || handle.done();
	}

	template<typename ParentPromise>
	void await_suspend( std::coroutine_handle<ParentPromise> parent ) noexcept
	{
		handle.promise().producer_ptr = parent.promise().producer_ptr;
		handle.promise().parent_handle = parent;
		handle.promise().parent_promise = &parent.promise();

		handle.promise().parent_promise->active_child_resume = [this](){ this->resume(); };

		this->resume();
	}

	void await_resume() noexcept
	{
	}
};

inline Director wait_for( int64_t frames )
{
	while ( frames > 0 )
	{
		co_yield {};
		--frames;
	}
}

template <typename... Tasks>
Director together(Tasks... tasks)
{
	while ( true )
	{
		bool any_running = false;

		(..., [&]()
		{
			if (!tasks.handle.done())
			{
				tasks.resume();
				if (!tasks.handle.done())
				{
					any_running = true;
				}
			}
		}());

		if (!any_running)
		{
			break;
		}

		co_yield {};
	}
	co_return;
}
