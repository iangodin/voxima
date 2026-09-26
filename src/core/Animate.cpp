#include "core/Animate.h"

#include "core/EaseCubic.h"
#include "core/Tween.h"
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
