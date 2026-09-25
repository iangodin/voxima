#include "WordEffects.h"

#include "Director.h"

#include <audio/TimedWord.h>
#include <visuals/Outline.h>
#include <visuals/Rectangle.h>

#include <algorithm>
#include <cmath>
#include <numbers>

namespace
{

// The nodes from `node` down to `target`, or nothing if target is not below it
bool find_path( const Visual &node, const Visual *target, std::vector<const Visual *> &path )
{
	path.push_back( &node );
	if ( &node == target )
	{
		return true;
	}
	if ( const auto *group = dynamic_cast<const Group *>( &node ) )
	{
		for ( const auto &child: *group )
		{
			if ( find_path( *child, target, path ) )
			{
				return true;
			}
		}
	}
	path.pop_back();
	return false;
}

// The box of everything drawn by a visual, in its parent's coordinates
AABB drawn_bounds( const Visual &visual )
{
	AABB box;
	for ( const auto &shape: to_outline( visual ) )
	{
		for ( const auto &contour: shape.contours )
		{
			box.add( on_curve_bounds( contour ) );
		}
	}
	return box;
}

// A box seen from the parent of the visual it is in
AABB in_parent( const AABB &box, const Visual &visual )
{
	AABB out;
	if ( box.empty() )
	{
		return out;
	}
	for ( Point corner: { box.min, Point{ box.max.x(), box.min.y() }, box.max, Point{ box.min.x(), box.max.y() } } )
	{
		out.add( place_in_parent( visual, corner ) );
	}
	return out;
}

// How much of the way to the new color: 0 is `a`, 1 is `b` (alpha is left alone)
Color mix( const Color &a, const Color &b, float t )
{
	return { a.r + ( b.r - a.r ) * t, a.g + ( b.g - a.g ) * t, a.b + ( b.b - a.b ) * t, 1.F };
}

void set_rgb( Visual &visual, const Color &c )
{
	visual.color.r = c.r;
	visual.color.g = c.g;
	visual.color.b = c.b;
}

// How long a word takes to settle into its "after" color once it has been said
constexpr double SETTLE_SECONDS = 0.25;

Scene reveal_effect( WordContext ctx )
{
	ctx.word.color.a = 0.F;
	co_await ctx.over_word( [&ctx]( float t ) { ctx.word.color.a = t; } );
}

Scene color_effect( WordContext ctx, Color before, Color during, Color after )
{
	set_rgb( ctx.word, before );
	// Reaches `during` a fifth of the way through the word, so it is lit while most of it is said
	co_await ctx.over_word( [&]( float t ) { set_rgb( ctx.word, mix( before, during, std::min( 1.F, t * 5.F ) ) ); } );
	co_await ctx.over_time( ctx.end_seconds, ctx.end_seconds + SETTLE_SECONDS, [&]( float t ) { set_rgb( ctx.word, mix( during, after, t ) ); } );
}

Scene pop_effect( WordContext ctx, float amount )
{
	// Scaling a group scales about its own origin, so it is moved to keep the word's centre where it was
	const Point centre = ctx.center();
	const Point home = ctx.word.position;
	const float scale_x = ctx.word.scale_x;
	const float scale_y = ctx.word.scale_y;

	co_await ctx.over_word(
		[&]( float t )
		{
			const float s = 1.F + ( amount - 1.F ) * std::sin( std::numbers::pi_v<float> * t );
			ctx.word.scale_x = scale_x * s;
			ctx.word.scale_y = scale_y * s;
			ctx.word.position = home + centre * ( 1.F - s );
		} );

	ctx.word.position = home;
	ctx.word.scale_x = scale_x;
	ctx.word.scale_y = scale_y;
}

Scene underline_effect( WordContext ctx, Color color, float thickness, float gap, bool keep )
{
	const AABB box = ctx.bounds();
	if ( box.empty() )
	{
		co_return; // a space: nothing to underline
	}

	auto bar = std::make_unique<Rectangle>();
	bar->color = color;
	bar->half_height = thickness / 2.F;
	bar->half_width = 0.F;
	const float y = box.min.y() - gap - thickness / 2.F;
	bar->position = { box.min.x(), y };
	Rectangle &rect = ctx.text.add( std::move( bar ) );

	co_await ctx.over_word(
		[&]( float t )
		{
			const float half = box.width() * t / 2.F;
			rect.half_width = half;
			rect.position = { box.min.x() + half, y };
		} );

	if ( !keep )
	{
		ctx.text.remove( &rect );
	}
}

Scene run_all( std::vector<WordEffect> effects, WordContext ctx )
{
	std::vector<Coroutine> running;
	for ( const auto &effect: effects )
	{
		running.push_back( effect( ctx ) );
	}
	co_await choreograph_all( std::move( running ) );
}

}

AABB WordContext::bounds() const
{
	std::vector<const Visual *> path;
	if ( !find_path( text, &word, path ) )
	{
		return {};
	}

	AABB box = drawn_bounds( word );
	// up through the groups between the word and the text (path.front() is the text, path.back() the word)
	for ( size_t k = path.size() - 1; k-- > 1; )
	{
		box = in_parent( box, *path[k] );
	}
	return box;
}

Point WordContext::center() const
{
	const AABB box = drawn_bounds( word );
	return box.empty() ? Point{} : Point{ ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F };
}

Scene WordContext::over_word( std::function<void( float )> fn ) const
{
	return over_time( start_seconds, end_seconds, std::move( fn ) );
}

Scene WordContext::over_time( double from, double to, std::function<void( float )> fn ) const
{
	return track_span( position, finished, from, to, std::move( fn ) );
}

Scene track_span(
	std::function<double()> position,
	std::function<bool()> finished,
	double from,
	double to,
	std::function<void( float )> callback )
{
	while ( !finished() && position() < from )
	{
		co_yield NextFrame{};
	}

	while ( !finished() )
	{
		const float t = progress( position(), from, to );
		if ( t >= 1.F )
		{
			break;
		}
		callback( t );
		co_yield NextFrame{};
	}
	callback( 1.F );
}

namespace word_effects
{

WordEffect all( std::vector<WordEffect> effects )
{
	return [effects]( WordContext ctx ) { return run_all( effects, ctx ); };
}

WordEffect reveal()
{
	return []( WordContext ctx ) { return reveal_effect( ctx ); };
}

WordEffect color( Color before, Color during, Color after )
{
	return [=]( WordContext ctx ) { return color_effect( ctx, before, during, after ); };
}

WordEffect pop( float scale )
{
	return [=]( WordContext ctx ) { return pop_effect( ctx, scale ); };
}

WordEffect underline( Color color, float thickness, float gap, bool keep )
{
	return [=]( WordContext ctx ) { return underline_effect( ctx, color, thickness, gap, keep ); };
}

}
