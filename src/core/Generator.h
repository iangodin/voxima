
#pragma once

#include <coroutine>
#include <exception>

template<typename T>
struct Generator
{
	struct promise_type
	{
		T current_value;

		Generator get_return_object()
		{
			return Generator{ std::coroutine_handle<promise_type>::from_promise( *this ) };
		}

		std::suspend_always initial_suspend()
		{
			return {};
		}

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

		std::suspend_always yield_value( T value )
		{
			current_value = std::move( value );
			return {};
		}
	};

	std::coroutine_handle<promise_type> handle;

	Generator( std::coroutine_handle<promise_type> h )
		: handle( h )
	{
	}

	~Generator()
	{
		if ( handle )
		{
			handle.destroy();
		}
	}

	Generator( const Generator & ) = delete;
	Generator &operator=( const Generator & ) = delete;

	Generator( Generator &&other ) noexcept
		: handle( other.handle )
	{
		other.handle = nullptr;
	}

	Generator &operator=( Generator &&other ) noexcept
	{
		if ( this != &other )
		{
			if ( handle )
			{
				handle.destroy();
			}
			handle = other.handle;
			other.handle = nullptr;
		}
		return *this;
	}

	bool next()
	{
		if ( handle )
		{
			handle.resume();
		}
		return !handle.done();
	}

	const T &value() const
	{
		return handle.promise().current_value;
	}
};
