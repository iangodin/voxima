
#pragma once

#include <cstddef>
#include <exception>
#include <utility>

template<typename T>
class ObserverPtr
{
public:
	class BadObserverAccess : public std::exception
	{
	public:
		// Default constructor
		constexpr BadObserverAccess() noexcept = default;

		// Destructor (required to be noexcept)
		~BadObserverAccess() noexcept override = default;

		// Copy and move operations
		BadObserverAccess( const BadObserverAccess & ) noexcept = default;
		BadObserverAccess &operator=( const BadObserverAccess & ) noexcept = default;

		// The message returned when the exception is caught
		[[nodiscard]] const char *what() const noexcept override
		{
			return "BadObserverAccess: referenced an empty ObserverPtr";
		}
	};

	constexpr ObserverPtr() noexcept = default;
	constexpr ObserverPtr( std::nullptr_t ) noexcept
		: _ptr( nullptr )
	{
	}

	constexpr explicit ObserverPtr( T *p ) noexcept
		: _ptr( p )
	{
	}

	constexpr explicit ObserverPtr( T &p ) noexcept
		: _ptr( &p )
	{
	}

	constexpr ObserverPtr( const ObserverPtr & ) noexcept = default;
	constexpr ObserverPtr( ObserverPtr && ) noexcept = default;

	constexpr ObserverPtr &operator=( const ObserverPtr & ) noexcept = default;
	constexpr ObserverPtr &operator=( ObserverPtr && ) noexcept = default;

	constexpr ObserverPtr &operator=( T &object ) noexcept
	{
		_ptr = &object;
		return *this;
	}

	constexpr bool has_value() const noexcept
	{
		return _ptr != nullptr;
	}

	constexpr explicit operator bool() const noexcept
	{
		return has_value();
	}

	constexpr T &value() const
	{
		if ( !has_value() )
		{
			throw BadObserverAccess();
		}
		return *_ptr;
	}

	template<typename U>
	constexpr T *value_or( U &&default_value ) const
	{
		return has_value() ? _ptr : static_cast<T *>( std::forward<U>( default_value ) );
	}

	constexpr T *get() const noexcept
	{
		return _ptr;
	}

	constexpr T &operator*() const
	{
		if ( !has_value() )
		{
			throw BadObserverAccess();
		}
		return *_ptr;
	}

	constexpr T *operator->() const
	{
		if ( !has_value() )
		{
			throw BadObserverAccess();
		}
		return _ptr;
	}

	constexpr void reset() noexcept
	{
		_ptr = nullptr;
	}

private:
	T *_ptr = nullptr;
};

template<typename T>
constexpr ObserverPtr<T> make_observer( T *p ) noexcept
{
	return ObserverPtr<T>( p );
}
