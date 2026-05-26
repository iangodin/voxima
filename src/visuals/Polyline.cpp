
#include "Polyline.h"

#include <algorithm>
#include <iterator>
#include <cassert>

////////////////////////////////////////

void Polyline::addQuadratic( const Point &p1, const Point &p2, const Point &p3 )
{
	Point c1 = ( p1 + 2.F * p2 ) / 3.F;
	Point c2 = ( p3 + 2.F * p2 ) / 3.F;
	addCubic( p1, c1, c2, p3 );
}

////////////////////////////////////////

void Polyline::addCubic( const Point &p1, const Point &p2, const Point &p3, const Point &p4 )
{
	struct Curve{ Point a, b, c, d; };

	auto flatness = []( const Curve &c ) -> float
	{
		const Point u = c.b * 3.F - c.a * 2.F - c.d;
		const Point v = c.c * 3.F - c.d * 2.F - c.a;
		const float x = std::max( u.x() * u.x(), v.x() * v.x() );
		const float y = std::max( u.y() * u.y(), v.y() * v.y() );
		return x + y;
	};

	std::vector<Curve> stack;
	stack.reserve( 16 );
	stack.push_back( { p1, p2, p3, p4 } );

	while ( !stack.empty() )
	{
		Curve c = stack.back();
		stack.pop_back();

		if ( flatness( c ) < 1.F / ( RESOLUTION * RESOLUTION ) )
		{
			addPoint( c.d );
		}
		else
		{
            const Point p01 = ( c.a + c.b ) / 2.F;
            const Point p23 = ( c.c + c.d ) / 2.F;
            const Point p012 = ( c.a + c.b * 2.F + c.c ) / 4.F;
            const Point p123 = ( c.b + c.c * 2.F + c.d ) / 4.F;
            const Point p0123 = ( c.a + c.b * 3.F + c.c * 3.F + c.d ) / 8.F;

			stack.push_back( { p0123, p123, p23, c.d } ); // right side
			stack.push_back( { c.a, p01, p012, p0123 } ); // left side
		}
	}
}

////////////////////////////////////////

void Polyline::addArc( const Point &center, float angle )
{
	assert( !mPoints.empty() );

	constexpr float pi = 3.14159265358979323846F;

	Point start = mPoints.back();
	if ( start != center )
	{
		float r = std::max( 1.F / RESOLUTION, std::sqrt( center.distanceSquaredTo( start ) ) );
		float deltaAngle = std::acos( 1.F - ( 1.F / RESOLUTION ) / r ) * 180.F / pi;
		size_t N = std::max( size_t{ 1 }, static_cast<size_t>( std::abs( angle ) / deltaAngle ) );

		for ( size_t i = 0; i < N; ++i )
		{
			Point p = start.rotatedAround( center, angle * static_cast<float>( i + 1 ) / static_cast<float>( N ) );
			mPoints.push_back( p );
		}
	}
}

////////////////////////////////////////

Area Polyline::calculateBounds( void ) const
{
	Area bounds;
	if ( !mPoints.empty() )
	{
		bounds = Area{ mPoints.front(), { 0.F, 0.F } };
		for ( auto &point: mPoints )
			bounds = bounds.extended( point );
	}
	return bounds;
}

////////////////////////////////////////

void Polyline::saveSVG( std::ostream &out, float width, const std::string &color ) const
{
	if ( mClosed )
		out << "<polygon points = \"";
	else
		out << "<polyline points = \"";
	std::copy( begin(), end(), std::ostream_iterator<Point>( out, " " ) );
	out << "\" stroke=\"" << color << "\" stroke-width=\"" << width << "\" fill=\"none\" />";
}

////////////////////////////////////////

void Polyline::saveSVGPointNumbers( std::ostream &out, float width, const std::string &color ) const
{
	for ( size_t i = 0; i < mPoints.size(); ++i )
	{
		const auto &p = mPoints[i];
		out << "<text x=\"" << p.x() << "\" y=\"" << p.y() << "\" stroke=\"" << width << "\" fill=\"" << color << "\" font-size=\"5\">" << i << "</text>\n";
	}
}

////////////////////////////////////////

void Polyline::print( std::ostream &out ) const
{
	std::copy( begin(), end(), std::ostream_iterator<Point>( out, "; " ) );
	if ( closed() )
		out << "closed";
}

////////////////////////////////////////

