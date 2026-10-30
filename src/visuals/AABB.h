#pragma once

#include "visuals/Point.h"

#include <algorithm>
#include <limits>

// Axis-aligned box. A default-constructed box is empty and grows as points are added.
struct AABB
{
	Point min = { std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
	Point max = { std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest() };

	[[nodiscard]] bool empty() const { return min.x() > max.x() || min.y() > max.y(); }

	void add( Point p )
	{
		min = { std::min( min.x(), p.x() ), std::min( min.y(), p.y() ) };
		max = { std::max( max.x(), p.x() ), std::max( max.y(), p.y() ) };
	}

	void add( const AABB &other )
	{
		if ( !other.empty() )
		{
			add( other.min );
			add( other.max );
		}
	}

	[[nodiscard]] AABB moved( Point delta ) const
	{
		AABB box;
		if ( !empty() )
		{
			box.add( min + delta );
			box.add( max + delta );
		}
		return box;
	}

	AABB &operator*=( float scale )
	{
		if ( !empty() )
		{
			min *= scale;
			max *= scale;
		}
		return *this;
	}

	[[nodiscard]] float width() const { return empty() ? 0.F : max.x() - min.x(); }
	[[nodiscard]] float height() const { return empty() ? 0.F : max.y() - min.y(); }
};
