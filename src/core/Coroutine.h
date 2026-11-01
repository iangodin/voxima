
#pragma once

#include "Production.h"

#include <coroutine>
#include <exception>
#include <utility>

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

		// Kept for whoever is waiting for this one: see await_resume() and resume()
		void unhandled_exception( void )
		{
			exception = std::current_exception();
		}

		void return_void( void )
		{
		}

		std::suspend_always yield_value( NextFrame )
		{
            return {};
        }
	};

	std::coroutine_handle<promise_type> handle = {};

	Coroutine() = default;

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

	Coroutine &operator=( Coroutine &&other )
	{
		std::swap( handle, other.handle );
		return *this;
	}

	void resume( void )
	{
		// (found now: one that is waited for can be gone once it has run, as its finishing lets what waits for it carry on)
		const bool waited_for = handle.promise().parent_promise != nullptr;
		if ( handle.promise().active_child_resume )
		{
			handle.promise().active_child_resume();
		}
		else
		{
			handle.resume();
		}
		// What goes wrong in a scene goes to what waits for it (see await_resume()), and from the outermost one, to whoever called
		// this: the producer, or a choreograph
		if ( !waited_for )
		{
			rethrow();
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

	// A scene that throws throws where it is waited for, as if it were a function, so a scene can catch it there
	void await_resume()
	{
		rethrow();
	}

private:
	void rethrow()
	{
		if ( handle && handle.promise().exception )
		{
			std::rethrow_exception( std::exchange( handle.promise().exception, nullptr ) );
		}
	}
};

