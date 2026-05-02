
#pragma once

#include <coroutine>
#include <cstdint>
#include <functional>

class Producer;

struct NextFrame
{
};

struct ProductionPromise
{
	Producer *producer_ptr = nullptr;
	ProductionPromise *parent_promise = nullptr;
	std::coroutine_handle<> parent_handle = nullptr;
	std::function<void(void)> active_child_resume = nullptr;
};

struct ReturnToParent
{
	bool await_ready( void ) noexcept
	{
		return false;
	}

	template<typename PromiseType>
	std::coroutine_handle<> await_suspend( std::coroutine_handle<PromiseType> handle ) noexcept
	{
		if ( handle.promise().parent_promise )
		{
			handle.promise().parent_promise->active_child_resume = nullptr;
		}

		auto &h = handle.promise().parent_handle;
		if ( h )
		{
			return h;
		}
		return std::noop_coroutine();
	}

	void await_resume( void ) noexcept
	{
	}
};

struct CurrentProducer
{
	Producer *producer = nullptr;

	bool await_ready() const noexcept
	{
		return false;
	}

	template<typename PromiseType>
	bool await_suspend( std::coroutine_handle<PromiseType> h ) noexcept
	{
		if constexpr (requires { h.promise().producer_ptr; })
		{
			producer = h.promise().producer_ptr;
		}
		return false;
	}

	Producer &await_resume() const noexcept
	{
		return *producer;
	}
};

