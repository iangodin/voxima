#include "core/Animate.h"

#include "core/EaseCubic.h"
#include "core/Director.h"
#include "core/Tween.h"
#include "core/Wait.h"
#include "visuals/Outline.h"
#include "visuals/Path.h"
#include "visuals/Placement.h"
#include "visuals/Trim.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{

float eased( const EaseFunction &ease, float t )
{
	return ease ? ease( t ) : ease_cubic_out( t );
}

// Show the visual at `amount` of its alpha
struct Alpha
{
	float color;
	float stroke;

	explicit Alpha( const Visual &visual ) :
		color( visual.color.a ),
		stroke( visual.stroke_color.a )
	{
	}

	void set( Visual &visual, float amount ) const
	{
		amount = std::clamp( amount, 0.F, 1.F );
		visual.color.a = color * amount;
		visual.stroke_color.a = stroke * amount;
	}
};

// Scaled by `factor` (of what it is now) with the center of its box staying where it is
struct CenteredScale
{
	Point position;
	Point offset; // from its origin to the center of its box
	float scale_x;
	float scale_y;

	explicit CenteredScale( const Visual &visual ) :
		position( visual.position ),
		scale_x( visual.scale_x ),
		scale_y( visual.scale_y )
	{
		offset = point_at( visual, Anchor::CENTER ) - visual.position;
	}

	void set( Visual &visual, float factor ) const
	{
		visual.scale_x = scale_x * factor;
		visual.scale_y = scale_y * factor;
		// (the box is as drawn before scaling, so the center's offset scales with it)
		const Point center = position + Point{ offset.x() * scale_x, offset.y() * scale_y };
		visual.position = center - Point{ offset.x() * visual.scale_x, offset.y() * visual.scale_y };
	}
};


// A rounded rectangle as a closed contour, starting at the top of its left side and going round (clockwise on screen)
Contour rounded_box( const AABB &box, float radius )
{
	const float r = std::min( { radius, ( box.max.x() - box.min.x() ) / 2.F, ( box.max.y() - box.min.y() ) / 2.F } );
	const float x0 = box.min.x();
	const float x1 = box.max.x();
	const float y0 = box.min.y();
	const float y1 = box.max.y();

	Contour c;
	c.start = { x0, y1 - r };
	Point from = c.start;
	auto line = [&]( Point to )
	{
		c.segs.push_back( { from, to, to } );
		from = to;
	};
	// A quarter circle to `to`, round the corner: its control points are most of the way to the corner
	auto corner = [&]( Point corner_point, Point to )
	{
		constexpr float K = 0.5523F;
		c.segs.push_back( { from + ( corner_point - from ) * K, to + ( corner_point - to ) * K, to } );
		from = to;
	};
	corner( { x0, y1 }, { x0 + r, y1 } );
	line( { x1 - r, y1 } );
	corner( { x1, y1 }, { x1, y1 - r } );
	line( { x1, y0 + r } );
	corner( { x1, y0 }, { x1 - r, y0 } );
	line( { x0 + r, y0 } );
	corner( { x0, y0 }, { x0, y0 + r } );
	line( { x0, y1 - r } );
	return c;
}

// The color asked for, else the palette's yellow (a stage with no theme has the plain one)
Color pointing_color( const Stage &stage, const PointOutOptions &options )
{
	if ( options.color )
	{
		return *options.color;
	}
	return stage.theme ? stage.theme->palette().yellow : Color{ 1.F, 0.85F, 0.2F, 1.F };
}

AABB padded( const AABB &box, float by )
{
	return AABB{ { box.min.x() - by, box.min.y() - by }, { box.max.x() + by, box.max.y() + by } };
}

}

Scene fade_in( const Stage &stage, Visual &visual, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Alpha alpha( visual );
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); }, [&]( float t ) { alpha.set( visual, t ); } );
}

Scene fade_out( const Stage &stage, Visual &visual, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Alpha alpha( visual );
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); }, [&]( float t ) { alpha.set( visual, 1.F - t ); } );
}

Scene fade_to( const Stage &stage, Visual &visual, float opacity, std::chrono::duration<double> duration, EaseFunction ease )
{
	const float from = visual.opacity;
	const float to = std::clamp( opacity, 0.F, 1.F );
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); }, [&]( float t ) { visual.opacity = from + ( to - from ) * t; } );
	visual.opacity = to;
}

Scene grow_in( const Stage &stage, Visual &visual, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Alpha alpha( visual );
	const CenteredScale scale( visual );
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); },
		[&]( float t )
		{
			scale.set( visual, std::max( t, 0.001F ) );
			alpha.set( visual, t );
		} );
}

Scene grow_from_edge( const Stage &stage, Visual &visual, Anchor edge, std::chrono::duration<double> duration, EaseFunction ease )
{
	const bool across = edge != Anchor::TOP && edge != Anchor::BOTTOM; // it grows along x
	const bool up = edge != Anchor::LEFT && edge != Anchor::RIGHT;     // and along y
	const Point position = visual.position;
	const float scale_x = visual.scale_x;
	const float scale_y = visual.scale_y;
	// (the box is as drawn before scaling, so the edge's offset scales with it)
	const Point offset = point_at( visual, edge ) - position;

	const auto set = [&]( float factor )
	{
		visual.scale_x = across ? scale_x * factor : scale_x;
		visual.scale_y = up ? scale_y * factor : scale_y;
		visual.position = {
			position.x() + offset.x() * ( scale_x - visual.scale_x ),
			position.y() + offset.y() * ( scale_y - visual.scale_y ) };
	};
	set( 0.001F );
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); }, [&]( float t ) { set( std::max( t, 0.001F ) ); } );
	visual.scale_x = scale_x;
	visual.scale_y = scale_y;
	visual.position = position;
}

Scene shrink_out( const Stage &stage, Visual &visual, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Alpha alpha( visual );
	const CenteredScale scale( visual );
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); },
		[&]( float t )
		{
			scale.set( visual, std::max( 1.F - t, 0.001F ) );
			alpha.set( visual, 1.F - t );
		} );
}

Scene slide_in( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Alpha alpha( visual );
	const Point rest = visual.position;
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); },
		[&]( float t )
		{
			visual.position = rest + offset * ( 1.F - t );
			alpha.set( visual, t );
		} );
}

Scene slide_out( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Alpha alpha( visual );
	const Point rest = visual.position;
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); },
		[&]( float t )
		{
			visual.position = rest + offset * t;
			alpha.set( visual, 1.F - t );
		} );
}

Scene move_by( const Stage &stage, Visual &visual, Point offset, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Point rest = visual.position;
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); }, [&]( float t ) { visual.position = rest + offset * t; } );
}

Scene indicate( const Stage &stage, Visual &visual, float factor, std::chrono::duration<double> duration )
{
	const CenteredScale scale( visual );
	co_await tween( stage, duration, []( float t ) { return t; },
		[&]( float t ) { scale.set( visual, 1.F + ( factor - 1.F ) * std::sin( t * std::numbers::pi_v<float> ) ); } );
	scale.set( visual, 1.F );
}

Scene draw_on( Stage &stage, Visual &visual, std::chrono::duration<double> duration, float trace, Group *parent )
{
	Group &into = parent ? *parent : stage.visuals;
	const Alpha alpha( visual );

	// One path for each shape, all its contours together; in the color the shape is seen in
	struct Traced
	{
		Path *path;
		std::vector<Contour> contours;
	};
	std::vector<Traced> traced;
	auto &overlay = into.add( std::make_unique<Group>() );
	for ( OutlineShape &shape: to_outline( visual, nullptr, stage.foreground ) )
	{
		if ( shape.contours.empty() )
		{
			continue;
		}
		auto path = std::make_unique<Path>();
		path->color = { 1.F, 1.F, 1.F, 0.F };
		path->stroke_color = shape.stroke.a > 0.F && shape.stroke_thickness > 0.F ? shape.stroke : shape.fill;
		path->stroke_thickness = shape.stroke_thickness > 0.F ? shape.stroke_thickness : 0.03F;
		path->stroke_dash = shape.stroke_thickness > 0.F ? shape.dash : StrokeDash{};
		path->join = LineJoin::ROUND;
		traced.push_back( { &overlay.add( std::move( path ) ), std::move( shape.contours ) } );
	}

	// (only now: the outlines have the colors the visual is seen in)
	alpha.set( visual, 0.F );
	trace = std::clamp( trace, 0.05F, 1.F );
	co_await tween( stage, duration, []( float t ) { return t; },
		[&]( float t )
		{
			const float along = std::min( t / trace, 1.F );
			const float fade = trace < 1.F ? std::clamp( ( t - trace ) / ( 1.F - trace ), 0.F, 1.F ) : ( t >= 1.F ? 1.F : 0.F );
			for ( Traced &shape: traced )
			{
				shape.path->commands.clear();
				for ( const Contour &contour: shape.contours )
				{
					for ( PathCommand &command: trimmed_commands( contour, ease_cubic_in_out( along ) ) )
					{
						shape.path->commands.push_back( command );
					}
				}
			}
			alpha.set( visual, fade );
			overlay.color.a = 1.F - fade;
			overlay.stroke_color.a = 1.F - fade;
		} );
	into.remove( &overlay );
}

Scene circumscribe( Stage &stage, Visual &visual, const PointOutOptions &options, std::chrono::duration<double> duration, Group *parent )
{
	Group &into = parent ? *parent : stage.visuals;
	const AABB box = box_of( visual );
	if ( box.empty() )
	{
		co_return;
	}
	const Contour outline = rounded_box( padded( box, options.padding ), options.radius );
	auto &pen = into.add( std::make_unique<Path>() );
	pen.color = { 1.F, 1.F, 1.F, 0.F };
	pen.stroke_color = pointing_color( stage, options );
	pen.stroke_thickness = options.thickness;
	pen.join = LineJoin::ROUND;
	const float base = pen.stroke_color.a;

	co_await tween( stage, duration, []( float t ) { return t; },
		[&]( float t )
		{
			const float along = std::min( t / 0.6F, 1.F );
			pen.commands = trimmed_commands( outline, ease_cubic_in_out( along ) );
			pen.stroke_color.a = base * ( 1.F - std::clamp( ( t - 0.6F ) / 0.4F, 0.F, 1.F ) );
		} );
	into.remove( &pen );
}

Scene flash( Stage &stage, Visual &visual, const PointOutOptions &options, std::chrono::duration<double> duration, Group *parent )
{
	Group &into = parent ? *parent : stage.visuals;
	const AABB box = box_of( visual );
	if ( box.empty() )
	{
		co_return;
	}
	auto &ring = into.add( std::make_unique<Path>() );
	ring.color = { 1.F, 1.F, 1.F, 0.F };
	ring.stroke_color = pointing_color( stage, options );
	ring.stroke_thickness = options.thickness;
	ring.join = LineJoin::ROUND;
	const float base = ring.stroke_color.a;

	co_await tween( stage, duration, []( float t ) { return t; },
		[&]( float t )
		{
			const float spread = options.padding * ( 0.5F + 2.5F * ease_cubic_out( t ) );
			ring.commands = trimmed_commands( rounded_box( padded( box, spread ), options.radius + spread - options.padding ), 1.F );
			ring.stroke_color.a = base * ( 1.F - t );
		} );
	into.remove( &ring );
}

Scene move_along( const Stage &stage, Visual &visual, const Path &path, std::chrono::duration<double> duration, EaseFunction ease, bool turn )
{
	const Contour contour = contour_of( path );
	if ( contour.segs.empty() )
	{
		co_return;
	}
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); },
		[&]( float t )
		{
			Point heading;
			visual.position = point_along( contour, t, &heading );
			if ( turn )
			{
				visual.rotation = std::atan2( heading.y(), heading.x() ) * 180.F / std::numbers::pi_v<float>;
			}
		} );
}

Scene turn_about( const Stage &stage, Visual &visual, Point center, float degrees, std::chrono::duration<double> duration, EaseFunction ease )
{
	const Point start = visual.position - center;
	const float rotation = visual.rotation;
	co_await tween( stage, duration, [&]( float t ) { return eased( ease, t ); },
		[&]( float t )
		{
			const float angle = degrees * t * std::numbers::pi_v<float> / 180.F;
			const float c = std::cos( angle );
			const float s = std::sin( angle );
			visual.position = center + Point{ start.x() * c - start.y() * s, start.x() * s + start.y() * c };
			visual.rotation = rotation + degrees * t;
		} );
}

namespace
{
Scene after( const Stage &stage, std::chrono::duration<double> delay, Scene scene )
{
	// Its first frame now: a verb that brings something in starts it hidden, so it is not seen before its turn
	if ( !scene.done() )
	{
		scene.resume();
	}
	co_await wait_for( stage, delay );
	co_await std::move( scene );
}
}

Scene stagger( const Stage &stage, std::vector<Visual *> visuals, std::chrono::duration<double> delay, std::function<Scene( Visual & )> verb )
{
	std::vector<Coroutine> running;
	for ( size_t i = 0; i < visuals.size(); ++i )
	{
		running.push_back( after( stage, delay * static_cast<double>( i ), verb( *visuals[i] ) ) );
	}
	co_await choreograph_all( std::move( running ) );
}

std::vector<Visual *> children_of( const Group &group )
{
	std::vector<Visual *> children;
	for ( const auto &child: group )
	{
		children.push_back( child.get() );
	}
	return children;
}
