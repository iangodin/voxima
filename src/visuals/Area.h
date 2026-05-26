
#pragma once

#include "Point.h"
#include "Size.h"

#include <algorithm>

#undef min
#undef max

//====================================//

/// @brief 2D rectangular area with a position and a size.
class Area
{
public:

	/// @name Constructors
	/// @{

	/// @brief Default constructor.
	Area( void ) = default;

	/// @brief Parameterized constructor.
	///
	/// @param x X position of the area.
	/// @param y Y position of the area.
	/// @param width Width of the area.
	/// @param height Height of the area.
	Area( float x, float y, float width, float height )
		: mPosition( x, y ), mSize( width, height )
	{
	}

	/// @brief Parameterized constructor.
	///
	/// @param position Position of the area.
	/// @param size Size of the area.
	Area( Point position, Size size )
		: mPosition( position ), mSize( size )
	{
	}

	/// @}

	/// @brief Queries
	/// @{

	/// @brief Position of the area.
	const Point &position( void ) const { return mPosition; }

	/// @brief Size of the area.
	const Size &size( void ) const { return mSize; }

	/// @brief X position of the area.
	float x( void ) const { return mPosition.x(); }

	/// @brief Y position of the area.
	float y( void ) const { return mPosition.y(); }

	/// @brief Width of the area.
	float width( void ) const { return mSize.width(); }

	/// @brief Height of the area.
	float height( void ) const { return mSize.height(); }

	/// @brief Left most side of the area.
	float left( void ) const { return mPosition.x(); }

	/// @brief Top most side of the area.
	float top( void ) const { return mPosition.y(); }

	/// @brief Right most side of the area.
	float right( void ) const { return mPosition.x() + mSize.width(); }

	/// @brief Following most side of the area.
	float following( void ) const { return mPosition.x() + mSize.width(); }

	/// @brief Bottom most side of the area.
	float bottom( void ) const { return mPosition.y() + mSize.height(); }

	/// @brief Below the bottom
	float below( void ) const { return mPosition.y() + mSize.height(); }

	/// @brief Center X position.
	float centerX( void ) const { return mPosition.x() + mSize.width() / 2.F; }

	/// @brief Center Y position.
	float centerY( void ) const { return mPosition.y() + mSize.height() / 2.F; }

	/// Top-left corner of the area.
	Point topLeft( void ) const { return mPosition; }

	/// Top-right corner of the area.
	Point topRight( void ) const { return mPosition + Point{ mSize.width(), 0 }; }

	/// Bottom-left corner of the area.
	Point bottomLeft( void ) const { return mPosition + Point{ 0, mSize.height() }; }

	/// Bottom-right corner of the area.
	Point bottomRight( void ) const { return mPosition + Point{ mSize.width(), mSize.height() }; }

	/// Center-left side of the area.
	Point centerLeft( void ) const { return mPosition + Point{ 0, mSize.height()/2.F }; }

	/// Center-right side of the area.
	Point centerRight( void ) const { return mPosition + Point{ mSize.width(), mSize.height()/2.F }; }

	/// Center-top side of the area.
	Point centerTop( void ) const { return mPosition + Point{ mSize.width()/2.F, 0 }; }

	/// Center-bottom side of the area.
	Point centerBottom( void ) const { return mPosition + Point{ mSize.width()/2.F, mSize.height() }; }

	/// Center of the area.
	Point center( void ) const { return mPosition + Point{ mSize.width()/2.F, mSize.height()/2.F }; }


	/// Check if a point is inside the area.
	///
	/// @param p Point to check.
	bool contains( const Point &p ) const
	{
		return p.x() >= left() && p.x() < right() && p.y() >= top() && p.y() < bottom();
	}

	/// Check if an area is inside this area.
	///
	/// @param p Point to check.
	bool containsHorizontal( const Area &a ) const
	{
		return a.left() >= left() && a.right() < right();
	}

	bool containsVertical( const Area &a ) const
	{
		return a.top() >= top() && a.bottom() < bottom();
	}

	/// Check if an area is inside this area.
	///
	/// @param a Area to check if it fits inside this.
	bool contains( const Area &a ) const
	{
		return containsHorizontal( a ) && containsVertical( a );
	}

	////////////////////////////////////////
	///!! Operators
	////////////////////////////////////////

	/// Check if this area is equal to the other area.
	///
	/// @param other Other area to compare with.
	bool operator==( const Area &other ) const
	{
		return position() == other.position() && size() == other.size();
	}

	/// Check if this area is not equal to the other area.
	///
	/// @param other Other area to compare with.
	bool operator!=( const Area &other ) const
	{
		return !( *this == other );
	}

	////////////////////////////////////////
	///!! Alterations
	////////////////////////////////////////

	/// New area moved to the given X position.
	///
	/// @param newX New X position.
	Area withX( float newX ) const
	{
		return Area{ mPosition.withX( newX ), mSize };
	}

	/// New area moved to the given Y position.
	///
	/// @param newY New Y position.
	Area withY( float newY ) const
	{
		return Area{ mPosition.withY( newY ), mSize };
	}

	/// New area with the given width.
	///
	/// @param newWidth New width.
	Area centerWidth( float newWidth ) const
	{
		float extra = width() - std::abs( newWidth );
		return Area{
			mPosition.movedBy( extra/2.F, 0.F ),
			mSize.withWidth( std::abs( newWidth ) ) };
	}

	/// New area with the given height.
	///
	/// @param newHeight New height.
	Area centerHeight( float newHeight ) const
	{
		float extra = height() - std::abs( newHeight );
		return Area{
			mPosition.movedBy( 0.F, extra/2.F ),
			mSize.withHeight( std::abs( newHeight ) ) };
	}

	/// New area with the given width.
	///
	/// @param newWidth New width.
	Area centerSize( float newWidth, float newHeight ) const
	{
		float extraW = width() - std::abs( newWidth );
		float extraH = height() - std::abs( newHeight );
		return Area{
			mPosition.movedBy( extraW/2, extraH/2 ),
			Size{ std::abs( newWidth ), std::abs( newHeight ) } };
	}

	/// New area with the given width.
	///
	/// @param newWidth New width.
	Area withWidth( float newWidth ) const
	{
		if ( newWidth < 0 )
		{
			return Area{
				mPosition.movedBy( mSize.width() + newWidth, 0 ),
				mSize.withWidth( -newWidth ) };
		}
		else
			return Area{ mPosition, mSize.withWidth( newWidth ) };
	}

	Area leftOf( float x ) const
	{
		return Area { mPosition, Size{ x - left(), height() } };
	}

	Area rightOf( float x ) const
	{
		return Area { mPosition.withX( x ), Size{ right() - x, height() } };
	}

	/// New area with the given height.
	///
	/// @param newHeight New height.
	Area withHeight( float newHeight ) const
	{
		if( newHeight < 0 )
		{
			return Area{
				mPosition.movedBy( 0, mSize.height() + newHeight ),
				mSize.withHeight( -newHeight ) };
		}
		else
			return Area{ mPosition, mSize.withHeight( newHeight ) };
	}

	/// New area with the given width and height.
	///
	/// @param newWidth New width.
	/// @param newHeight New height.
	Area withHeight( float newWidth, float newHeight ) const
	{
		return withWidth( newWidth ).withHeight( newHeight );
	}

	/// New area with the left side at the given location.
	///
	/// @param newX1 New left side.
	Area withX1( float newX1 ) const
	{
		return fromCoordinates( newX1, top(), right(), bottom() );
	}

	/// New area with the right side at the given location.
	///
	/// @param newX2 New right side.
	Area withX2( float newX2 ) const
	{
		return fromCoordinates( left(), top(), newX2, bottom() );
	}

	/// New area with the top side at the given location.
	///
	/// @param newY1 New top side.
	Area withY1( float newY1 ) const
	{
		return fromCoordinates( left(), newY1, right(), bottom() );
	}

	/// New area with the bottom side at the given location.
	///
	/// @param newY2 New bottom side.
	Area withY2( float newY2 ) const
	{
		return fromCoordinates( left(), top(), right(), newY2 );
	}

	/// Extend this area to include the point.
	///
	/// @param point Point to include.
	Area extended( const Point &point ) const
	{
		return fromCoordinates(
			std::min( left(), point.x() ),
			std::min( top(), point.y() ),
			std::max( right(), point.x() ),
			std::max( bottom(), point.y() ) );
	}


	/// Extend this area to include the other area.
	///
	/// @param area Area to include.
	Area extended( const Area &other ) const
	{
		return fromCoordinates(
			std::min( left(), other.left() ),
			std::min( top(), other.top() ),
			std::max( right(), other.right() ),
			std::max( bottom(), other.bottom() ) );
	}

	/// The given area clamped to be inside this area.
	///
	/// @param area Area to clamp.
	Area clamp( const Area &area ) const
	{
		auto clamp = []( const float x, const float min, const float max )
		{
			return std::min( max, std::max( min, x ) );
		};

		return fromCoordinates(
			clamp( area.left(), left(), right() ),
			clamp( area.top(), top(), bottom() ),
			clamp( area.right(), left(), right() ),
			clamp( area.bottom(), top(), bottom() ) );
	}

	/// This area shrunk by the given amount on all sides.
	///
	/// @param amount Amount to shrink by.
	Area shrunkBy( float amount ) const
	{
		return Area{
			mPosition.movedBy( amount, amount ),
			mSize.shrunkBy( amount * 2 )
		};
	}

	/// This area shrunk by the given amounts on all sides.
	///
	/// @param amountHorizontal Amount to shrink by horizontally.
	/// @param amountVertical Amount to shrink by vertically.
	Area shrunkBy( float amountHorizontal, float amountVertical ) const
	{
		return Area{
			mPosition.movedBy( amountHorizontal, amountVertical ),
			mSize.shrunkBy( amountHorizontal * 2, amountVertical * 2 )
		};
	}

	Point horizontalPosition( float numerator, float denomiator )
	{
		return Point {
			left() + width() * numerator / denomiator,
			centerY() };
	}

	Point verticalPosition( float numerator, float denomiator )
	{
		return Point {
			centerX(),
			top() + height() * numerator / denomiator
		};
	}

	/// @brief This area shrunk by the given amounts on each side.
	///
	/// @param amountLeft Amount to shrink the left side by.
	/// @param amountRight Amount to shrink the right side by.
	/// @param amountTop Amount to shrink the top side by.
	/// @param amountBottom Amount to shrink the bottom side by.
	Area shrunkBy( float amountLeft, float amountRight, float amountTop, float amountBottom ) const
	{
		return Area{
			mPosition.movedBy( amountLeft, amountTop ),
			mSize.shrunkBy( amountLeft + amountRight, amountTop + amountBottom )
		};
	}

	Area shrunkLeftBy( float amount ) const
	{
		return shrunkBy( amount, 0, 0, 0 );
	}

	Area shrunkRightBy( float amount ) const
	{
		return shrunkBy( 0, amount, 0, 0 );
	}

	Area shrunkTopBy( float amount ) const
	{
		return shrunkBy( 0, 0, amount, 0 );
	}

	Area shrunkBottomBy( float amount ) const
	{
		return shrunkBy( 0, 0, 0, amount );
	}

	/// @brief This area grown by the given amount on each side.
	///
	/// @param amountLeft Amount to grow the left side by.
	/// @param amountRight Amount to grow the right side by.
	/// @param amountTop Amount to grow the top side by.
	/// @param amountBottom Amount to grow the bottom side by.
	Area grownBy( float amountLeft, float amountRight, float amountTop, float amountBottom ) const
	{
		return Area{
			mPosition.movedBy( -amountLeft, -amountTop ),
			mSize.grownBy( amountLeft + amountRight, amountTop + amountBottom ) };
	}

	/// @brief This area grown by the given amount on each side.
	Area grownBy( float width, float height ) const
	{
		return grownBy( width, width, height, height );
	}

	Area grownBy( float size ) const
	{
		return grownBy( size, size );
	}

	/// @brief This area moved by the given amount.
	///
	/// @param dx Amount to change X.
	/// @param dy Amount to change Y.
	Area movedBy( float dx, float dy ) const
	{
		return Area{
			mPosition.movedBy( dx, dy ),
			mSize };
	}

	/// @brief This area moved to the given position.
	///
	/// @param newPosition New position for this area.
	Area movedTo( const Point &newPosition ) const
	{
		return Area{ newPosition, mSize };
	}

	/// @brief New area at the same position as this, but with the given size.
	///
	/// @param newSize New size for this area.
	///
	/// @return New area with the given size.
	Area resizedTo( const Size &newSize ) const
	{
		return Area{ mPosition, newSize };
	}

	/// @brief Create area based on the given coordinates.
	///
	/// @param x1 Left or right side of the area.
	/// @param y1 Top or bottom side of the area.
	/// @param x2 Other left or right side of the area.
	/// @param y2 Other top or bottom side of the area.
	static Area fromCoordinates( float x1, float y1, float x2, float y2 )
	{
		const float x = std::min( x1, x2 );
		const float y = std::min( y1, y2 );
		const float w = std::abs( x2 - x1 );
		const float h = std::abs( y2 - y1 );
		return Area{ x, y, w, h };
	}

	/// @name I/O
	/// @{

	/// @brief Print the area to the given output stream.
	///
	/// @param out Output stream.
	/// @param area Area to print.
	friend std::ostream &operator<<( std::ostream &out, const Area &area )
	{
		out << area.position() << '|' << area.size();
		return out;
	}

	/// @}

private:
	Point mPosition;
	Size mSize;
};

//====================================//

