
#pragma once

#include <cmath>
#include <iostream>
#include <limits>
#include <cstdint>


//====================================//

// Class: geometry::Point
//   A 2D point.
class Point
{
public:
////////////////////////////////////////
// Group: Constructors
////////////////////////////////////////

	// Constructor: Point
	//   Default constructor.
	Point( void ) = default;

	// Constructor: Point
	//   Parameterized constructor.
	//
	// Parameters:
	//   x - X position for this point.
	//   y - Y position for this point.
	Point( float x, float y )
		: _x{ x }, _y{ y }
	{
	}

////////////////////////////////////////
// Group: Comparisons
////////////////////////////////////////

	// Method: operator==
	//   Check if this point is equal to the other point.
	bool operator==( const Point &other ) const
	{
		return std::abs( other.x() - x() ) < 1e-4 && std::abs( other.y() - y() ) < 1e-4;
	}

	// Method: operator!=
	//   Check if this point is not equal to the other point.
	bool operator!=( const Point &other ) const
	{
		return !operator==( other );
	}

////////////////////////////////////////
// Group: Properties
////////////////////////////////////////

	// Method: x
	//   X position of this point.
	float x( void ) const
	{
		return _x;
	}

	// Method: y
	//   Y position of this point.
	float y( void ) const
	{
		return _y;
	}

////////////////////////////////////////
// Group: Alterations
////////////////////////////////////////

	// Method: movedBy
	//   This point moved by the given amount.
	Point movedBy( float dx, float dy ) const
	{
		return { _x + dx, _y + dy };
	}

	// Method: movedBy
	//   This point moved by the given amount.
	Point movedBy( const Point &delta ) const
	{
		return movedBy( delta.x(), delta.y() );
	}

	// Method: rotatedAround
	//   This point rotated around the center point by the given angle.
	Point rotatedAround( const Point &center, float angle )
	{
		const float s = std::sin( angle * pi / 180.F );
		const float c = std::cos( angle * pi / 180.F );

		const Point delta = (*this) - center;
		const Point rotated {
			std::round( delta.x() * c - delta.y() * s ),
			std::round( delta.x() * s + delta.y() * c ) };

		return center + rotated;
	}

	// Method: withX
	//   Replace X coordinate with newX.
	Point withX( float newX ) const
	{
		return Point{ newX, _y };
	}

	// Method: withY
	//   Replace Y coordinate with newY.
	Point withY( float newY ) const
	{
		return Point{ _x, newY };
	}

////////////////////////////////////////
// Group: Queries
////////////////////////////////////////

	// Method: distanceSquaredTo
	//   Distance from this point to the other point squared.
	float distanceSquaredTo( const Point &other ) const
	{
		return ( (*this) - other ).lengthSquared();
	}

	// Method: lengthSquared
	//   Distance from this point to the origina squared.
	float lengthSquared( void ) const
	{
		return _x * _x + _y * _y;
	}

////////////////////////////////////////
// Group: Operators
////////////////////////////////////////

	// Function: operator*
	//   Element-wise multiplication of two points a and b.
	Point &operator*=( float b )
	{
		_x *= b;
		_y *= b;
		return *this;
	}

	Point &operator+=( const Point &b )
	{
		_x += b.x();
		_y += b.y();
		return *this;
	}

	// Function: operator+
	//   Adding two points a and b.
	friend Point operator+( const Point &a, const Point &b )
	{
		return { a.x() + b.x(), a.y() + b.y() };
	}

	// Function: operator-
	//   Subtracting two points a and b.
	friend Point operator-( const Point &a, const Point &b )
	{
		return { a.x() - b.x(), a.y() - b.y() };
	}

	// Function: operator-
	//   Negation operator.
	friend Point operator-( const Point &a )
	{
		return { -a.x(), -a.y() };
	}

	// Function: operator*
	//   Element-wise multiplication of two points a and b.
	friend Point operator*( const Point &a, float b )
	{
		return { a.x() * b, a.y() * b };
	}

	// Function: operator*
	//   Multiplication of a point by a number.
	friend Point operator*( float a, const Point &b )
	{
		return { a * b.x(), a * b.y() };
	}

	// Function: operator/
	//   Division of a point by a number.
	friend Point operator/( const Point &a, float b )
	{
		return { a.x() / b, a.y() / b };
	}

	// Function: operator<<
	//   Print the point to the output stream.
	friend std::ostream &operator<<( std::ostream &out, const Point &point )
	{
		out << point.x() << ',' << point.y();
		return out;
	}

private:
	constexpr static const float pi = 3.14159265358979323846F;
	float _x = 0.F;
	float _y = 0.F;
};

//====================================//

