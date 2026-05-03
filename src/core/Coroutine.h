
#pragma once

#include "Production.h"

#include <coroutine>
#include <exception>

struct Coroutine
{
	struct promise_type : public ProductionPromise
	{
		Coroutine get_return_object( void )
		{
			return Coroutine{std::coroutine_handle<promise_type>::from_promise(*this)};
		}

		std::suspend_always initial_suspend( void )
		{
			return {};
		}

		ReturnToParent final_suspend( void ) noexcept
		{
			return {};
		}

		void unhandled_exception( void )
		{
			std::terminate();
		}

		void return_void( void )
		{
		}

		std::suspend_always yield_value( NextFrame )
		{
            return {};
        }
	};

	std::coroutine_handle<promise_type> handle;

	Coroutine( std::coroutine_handle<promise_type> h ) :
		handle(h)
	{
	}

	Coroutine( Coroutine &&other ) noexcept :
		handle(other.handle)
	{
		other.handle = nullptr;
	}

	~Coroutine( void )
	{
		if ( handle )
		{
			handle.destroy();
		}
	}

	void resume( void )
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

	bool done( void )
	{
		return !handle || handle.done();
	}

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

