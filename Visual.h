
#pragma once

#include "Point.h"
#include "Color.h"

struct VisualVisitor;

struct Visual
{
	Point position = { 0.F, 0.F };
	float rotation = 0.F;
	Color color = { 1.F, 1.F, 1.F, 1.F };

	float stroke_thickness = 0.F;
	Color stroke_color = { 1.F, 1.F, 1.F, 1.F }; // Black stroke by default

	virtual ~Visual() = default;
	virtual void accept( VisualVisitor &visitor ) const = 0;
};

