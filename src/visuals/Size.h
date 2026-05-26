
#pragma once

#include <iostream>
#include <cassert>
#include <algorithm>

#undef max

////////////////////////////////////////

// @brief 2D size.
class Size
{
public:
	/// @name Constructors.
	/// @{

	/// @brief Default constructor.
	///
	/// Creates a size of 0x0.
	Size( void ) = default;

	/// @brief Parameterized constructor.
	///
	/// @brief width Width for this size.
	/// @brief height Height for this size.
	Size( float width, float height )
		: _width{ width }, _height{ height }
	{
		invariants();
	}

	/// @}

	/// @name Properties..
	/// @{

	/// @brief The width of this size.
	float width( void ) const
	{
		return _width;
	}

	/// @brief The height of this size.
	float height( void ) const
	{
		return _height;
	}

	/// @}

	/// @name Queries
	/// @{

	/// @brief Check if the size is empty (0 in width or height).
	bool empty( void ) const
	{
		return _width == 0 || _height == 0;
	}

	/// @}

	/// @name Operators.
	/// @{

	/// @brief Check if this size is equal to the other size.
	///
	/// @param other Other size to compare against.
	bool operator==( const Size &other ) const
	{
		return std::abs( other.width() - width() ) < 1e-4
			&& std::abs( other.height() - height() ) < 1e-4;
	}

	/// @brief Check if this size is not equal to the other size.
	///
	/// @param other Other size to compare against.
	bool operator!=( const Size &other ) const
	{
		return !operator==( other );
	}

	/// @}

	/// @name Modifications.
	/// @{

	/// @brief The largest of this size and the other size.
	///
	/// @param other Other size to compare against.
	Size maximum( const Size &other ) const
	{
		return { std::max( other.width(), width() ), std::max( other.height(), height() ) };
	}

	/// @brief The largest of this height and the other height.
	///
	/// @param other Other size to compare against.
	Size maximumHeight( const Size &other ) const
	{
		return { width(), std::max( other.height(), height() ) };
	}

	/// @brief This size grown by the given amounts.
	///
	/// @param extraWidth Extra width to grow
	/// @param extraHeight Extra height to grow
	Size grownBy( float extraWidth, float extraHeight ) const
	{
		return Size{ _width + extraWidth, _height + extraHeight };
	}

	/// @brief This size grown by the given amount.
	Size grownBy( float amount ) const
	{
		return grownBy( amount, amount );
	}

	/// @brief This size shrunk by the given amounts.
	Size shrunkBy( float dw, float dh ) const
	{
		return Size{ _width - dw, _height - dh };
	}

	/// @brief This size shrunk by the given amount.
	Size shrunkBy( float amount ) const
	{
		return shrunkBy( amount, amount );
	}

	/// @brief This size with the given width.
	Size withWidth( float width ) const
	{
		return Size{ width, _height };
	}

	/// @brief This size with the given height.
	Size withHeight( float height ) const
	{
		return Size{ _width, height };
	}

	/// @brief Combine two sizes next to each other horizontally.
	Size beside( const Size &other ) const
	{
		return Size{
			width() + other.width(),
			std::max( height(), other.height() ) };
	}

	/// @brief Combine two sizes on top of each other vertically.
	Size atop( const Size &other ) const
	{
		return Size{
			std::max( width(), other.width() ),
			height() + other.height() };
	}

	/// @}

	/// @name I/O
	/// @{

	/// @brief Print a size to the output stream.
	friend std::ostream &operator<<( std::ostream &out, const Size &size )
	{
		out << size.width() << 'x' << size.height();
		return out;
	}

	/// @}

	/// @name Operators.
	/// @{

	/// @brief Scale the size by the given number.
	friend Size operator*( const Size &a, float b )
	{
		return { a.width() * b, a.height() * b };
	}

	/// @brief Scale the size by the given number.
	friend Size operator*( float a, const Size &b )
	{
		return { a * b.width(), a * b.height() };
	}

	/// @brief Divide size
	friend Size operator/( const Size &a, float b )
	{
		return { a.width() / b, a.height() / b };
	}

	/// @brief Add two sizes
	friend Size operator+( const Size &a, const Size &b )
	{
		return { a.width() + b.width(), a.height() + b.height() };
	}

	/// @}

private:
	void invariants( void ) const
	{
		assert( width() >= 0.F );
		assert( height() >= 0.F );
	}

	float _width = 0.F;
	float _height = 0.F;
};

////////////////////////////////////////

