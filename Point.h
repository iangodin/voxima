
#pragma once

struct Point
{
	float x = 0.F;
	float y = 0.F;
};

Point operator+( const Point &a, const Point &b )
{
	return { a.x + b.x, a.y + b.y };
}

