#include "visuals/Gradient.h"

#include "visuals/Bounds.h"
#include "visuals/CallbackVisitor.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{
Color blend( const Color &a, const Color &b, float t )
{
	return { a.r + ( b.r - a.r ) * t, a.g + ( b.g - a.g ) * t, a.b + ( b.b - a.b ) * t, a.a + ( b.a - a.a ) * t };
}

bool finite( Point p )
{
	return std::isfinite( p.x() ) && std::isfinite( p.y() );
}
}

Gradient Gradient::linear( Point start, Point end, std::vector<GradientStop> stops )
{
	Gradient out;
	out.start = start;
	out.end = end;
	out.stops = std::move( stops );
	return out;
}

Gradient Gradient::radial( Point center, float radius, std::vector<GradientStop> stops )
{
	return radial( center, radius, center, std::move( stops ) );
}

Gradient Gradient::radial( Point center, float radius, Point focus, std::vector<GradientStop> stops )
{
	Gradient out;
	out.kind = GradientKind::RADIAL;
	out.center = center;
	out.radius = radius;
	out.focus = focus;
	out.stops = std::move( stops );
	return out;
}

Gradient Gradient::across( const AABB &box, float degrees, std::vector<GradientStop> stops )
{
	const float radians = degrees * std::numbers::pi_v<float> / 180.F;
	const Point heading{ std::cos( radians ), std::sin( radians ) };
	const Point middle = ( box.min + box.max ) * 0.5F;
	const float reach = ( std::abs( heading.x() ) * ( box.max.x() - box.min.x() ) + std::abs( heading.y() ) * ( box.max.y() - box.min.y() ) ) * 0.5F;
	return linear( middle - heading * reach, middle + heading * reach, std::move( stops ) );
}

bool Gradient::drawable() const
{
	if ( stops.empty() || std::abs( transform.a * transform.d - transform.b * transform.c ) < 1e-12F )
	{
		return false;
	}
	if ( kind == GradientKind::LINEAR )
	{
		return finite( start ) && finite( end ) && ( end - start ).lengthSquared() > 1e-12F;
	}
	return finite( center ) && finite( focus ) && std::isfinite( radius ) && radius > 1e-6F;
}

std::vector<GradientStop> Gradient::ordered_stops() const
{
	std::vector<GradientStop> out = stops;
	for ( GradientStop &stop: out )
	{
		stop.offset = std::isfinite( stop.offset ) ? std::clamp( stop.offset, 0.F, 1.F ) : 0.F;
	}
	std::stable_sort( out.begin(), out.end(), []( const GradientStop &a, const GradientStop &b ) { return a.offset < b.offset; } );
	return out;
}

Color Gradient::sample( float offset ) const
{
	const std::vector<GradientStop> ordered = ordered_stops();
	if ( ordered.empty() )
	{
		return {};
	}
	if ( offset <= ordered.front().offset )
	{
		return ordered.front().color;
	}
	for ( size_t i = 1; i < ordered.size(); ++i )
	{
		if ( offset <= ordered[i].offset )
		{
			const float span = ordered[i].offset - ordered[i - 1].offset;
			return span > 0.F ? blend( ordered[i - 1].color, ordered[i].color, ( offset - ordered[i - 1].offset ) / span ) : ordered[i].color;
		}
	}
	return ordered.back().color;
}

Color Gradient::average() const
{
	const std::vector<GradientStop> ordered = ordered_stops();
	if ( ordered.empty() )
	{
		return {};
	}
	// Each stretch counts for its length; before the first stop and after the last, the color stays the same
	Color total{ 0.F, 0.F, 0.F, 0.F };
	const auto add = [&]( const Color &c, float length )
	{
		total.r += c.r * length;
		total.g += c.g * length;
		total.b += c.b * length;
		total.a += c.a * length;
	};
	add( ordered.front().color, ordered.front().offset );
	for ( size_t i = 1; i < ordered.size(); ++i )
	{
		add( blend( ordered[i - 1].color, ordered[i].color, 0.5F ), ordered[i].offset - ordered[i - 1].offset );
	}
	add( ordered.back().color, 1.F - ordered.back().offset );
	return total;
}

Gradient &Gradient::operator*=( float scale )
{
	start *= scale;
	end *= scale;
	center *= scale;
	focus *= scale;
	radius *= scale;
	transform.tx *= scale;
	transform.ty *= scale;
	return *this;
}

void fill_each_with_gradient( Visual &visual, float degrees, const std::vector<GradientStop> &stops )
{
	CallbackVisitor fill(
		[&]( Visual &leaf )
		{
			if ( dynamic_cast<Group *>( &leaf ) || dynamic_cast<Line *>( &leaf ) || dynamic_cast<Image *>( &leaf ) )
			{
				return;
			}
			// The box of its outline, in its own coordinates
			const Point position = leaf.position;
			const float scale_x = leaf.scale_x;
			const float scale_y = leaf.scale_y;
			leaf.position = { 0.F, 0.F };
			leaf.scale_x = leaf.scale_y = 1.F;
			BoundingBoxVisitor bounds;
			leaf.accept( bounds );
			leaf.position = position;
			leaf.scale_x = scale_x;
			leaf.scale_y = scale_y;
			if ( !bounds.bounds.empty() )
			{
				leaf.fill_gradient = Gradient::across( bounds.bounds, degrees, stops );
			}
		} );
	visual.accept( fill );
}
