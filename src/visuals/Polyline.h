
#pragma once

#include "Point.h"
#include "Area.h"

#include <iostream>
#include <vector>

//====================================//

class Polyline
{
public:
	static constexpr float RESOLUTION = 1000.0F;

    ////////////////////////////////////////
    ///!! Properties
    ////////////////////////////////////////

    /// Check if this polyline has no points.
	bool empty( void ) const { return mPoints.empty(); }

    /// Check if this polyline is closed.
	bool closed( void ) const { return mClosed; }

    /// The last point in this polyline.
	const Point &back( void ) const { return mPoints.back(); }

    /// The first point in this polyline.
	const Point &front( void ) const { return mPoints.front(); }

    /// The number of points in this polyline.
	size_t size( void ) const { return mPoints.size(); }

    ////////////////////////////////////////
    ///!! Iterable
    ////////////////////////////////////////

    /// Begin iterator.
	std::vector<Point>::iterator begin( void ) { return mPoints.begin(); }

    /// End iterator.
	std::vector<Point>::iterator end( void ) { return mPoints.end(); }

    /// Constant begin iterator.
	std::vector<Point>::const_iterator begin( void ) const { return mPoints.begin(); }

    /// Constant end iterator.
	std::vector<Point>::const_iterator end( void ) const { return mPoints.end(); }

    ////////////////////////////////////////
    ///!! Queries
    ////////////////////////////////////////

    /// Get ith point in this polyline.
    ///
    /// @param ith Index of point to get.
	const Point &operator[]( size_t ith ) const
	{
		return mPoints[ith];
	}

    /// Calculate outer bounds of this polyline.
	Area calculateBounds( void ) const;

    ////////////////////////////////////////
    ///!! Modifications
    ////////////////////////////////////////

    /// Close this polyline.
    ///
    /// Indicates that the last point connects back to the first point.
	void close( void ) { mClosed = true; }

    /// Add a point to this polyline.
	void addPoint( const Point &p ) { mPoints.push_back( p ); }

    /// Add a quadratic curve with the given points.
	void addQuadratic( const Point &p1, const Point &p2, const Point &p3 );

    /// Add a cubic curve with the given points.
	void addCubic( const Point &p1, const Point &p2, const Point &p3, const Point &p4 );

    /// Add an arc with the given center and angle.
    ///
    /// The arc starts at the current (last) point.
    /// Rotate clockwise around the center, by the given angle.
	void addArc( const Point &center, float angle );

    ////////////////////////////////////////
    ///!! Streaming
    ////////////////////////////////////////

	void saveSVG( std::ostream &out, float width = 1.F, const std::string &color = "black" ) const;
	void saveSVGPointNumbers( std::ostream &out, float width = 1.F, const std::string &color = "black" ) const;

	void print( std::ostream &out ) const;

	friend std::ostream &operator<<( std::ostream &out, const Polyline &polyline )
	{
		polyline.print( out );
		return out;
	}

private:
	bool mClosed = false;
	std::vector<Point> mPoints;
};

//====================================//

