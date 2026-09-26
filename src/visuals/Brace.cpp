#include "visuals/Brace.h"

#include <cmath>

namespace
{

struct Frame
{
	Point origin;
	Point along; // unit, from `from` toward `to`
	Point out;   // unit, toward the side the brace stands out to
	float length;
};

Frame frame_of( Point from, Point to, bool left )
{
	const Point d = to - from;
	const float length = std::sqrt( d.lengthSquared() );
	const Point along = length > 1e-6F ? d / length : Point{ 1.F, 0.F };
	Point out = { -along.y(), along.x() };
	if ( !left )
	{
		out = out * -1.F;
	}
	return { from, along, out, length };
}

Point at( const Frame &f, float x, float y )
{
	return f.origin + f.along * x + f.out * y;
}

}

std::unique_ptr<Path> make_brace( Point from, Point to, const BraceOptions &options )
{
	const Frame f = frame_of( from, to, options.left );
	const float half = f.length / 2.F;
	// The curls are quarter turns of radius r; never more than a quarter of the length, so they do not meet
	const float r = std::min( options.depth / 2.F, half / 2.F );
	constexpr float K = 0.5523F;

	auto path = std::make_unique<Path>();
	path->color = { 1.F, 1.F, 1.F, 0.F }; // a line, not a filled shape
	path->stroke_color = options.color;
	path->stroke_thickness = options.thickness;
	path->join = LineJoin::ROUND;

	auto &c = path->commands;
	c.reserve( 8 );
	// From the start: up a quarter turn to the run along, then a quarter turn to the point in the middle, and the same back
	c.push_back( MoveTo{ at( f, 0.F, 0.F ) } );
	c.push_back( CubicTo{ at( f, r, r ), at( f, 0.F, r * K ), at( f, r * ( 1.F - K ), r ) } );
	c.push_back( LineTo{ at( f, half - r, r ) } );
	c.push_back( CubicTo{ at( f, half, 2.F * r ), at( f, half - r * ( 1.F - K ), r ), at( f, half, r + r * ( 1.F - K ) ) } );
	c.push_back( CubicTo{ at( f, half + r, r ), at( f, half, r + r * ( 1.F - K ) ), at( f, half + r * ( 1.F - K ), r ) } );
	c.push_back( LineTo{ at( f, f.length - r, r ) } );
	c.push_back( CubicTo{ at( f, f.length, 0.F ), at( f, f.length - r * ( 1.F - K ), r ), at( f, f.length, r * K ) } );
	return path;
}

namespace
{

// The line along a side of a box, a gap out from it, and which side of it is away from the box
struct SideLine
{
	Point from;
	Point to;
	bool left;
};

SideLine side_line( const Visual &visual, Anchor side, float gap )
{
	const AABB box = box_of( visual );
	switch ( side )
	{
		case Anchor::TOP: return { { box.min.x(), box.max.y() + gap }, { box.max.x(), box.max.y() + gap }, true };
		case Anchor::BOTTOM: return { { box.min.x(), box.min.y() - gap }, { box.max.x(), box.min.y() - gap }, false };
		case Anchor::LEFT: return { { box.min.x() - gap, box.min.y() }, { box.min.x() - gap, box.max.y() }, true };
		default: return { { box.max.x() + gap, box.min.y() }, { box.max.x() + gap, box.max.y() }, false }; // RIGHT
	}
}

}

std::unique_ptr<Path> make_brace_beside( const Visual &visual, Anchor side, float gap, const BraceOptions &options )
{
	const SideLine line = side_line( visual, side, gap );
	BraceOptions o = options;
	o.left = line.left;
	return make_brace( line.from, line.to, o );
}

std::unique_ptr<Group> make_labeled_brace( Theme &theme, Point from, Point to, const std::string &label, const FontStyle &style, const BraceOptions &options )
{
	auto group = std::make_unique<Group>();
	group->add( make_brace( from, to, options ) );

	const Frame f = frame_of( from, to, options.left );
	const Point tip = at( f, f.length / 2.F, options.depth );
	auto &text = group->add( make_text( theme, label, style ) );

	// Its near edge (the one facing the brace) a gap out from the tip, its middle on the line out from it
	const Point target = tip + f.out * options.label_gap;
	const AABB box = box_of( text );
	const Point middle = { ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F };
	// The half-size of the box toward `out`
	const float reach = std::abs( f.out.x() ) * ( box.max.x() - box.min.x() ) / 2.F + std::abs( f.out.y() ) * ( box.max.y() - box.min.y() ) / 2.F;
	text.position = text.position + ( target + f.out * reach - middle );
	return group;
}

std::unique_ptr<Group> make_labeled_brace_beside( Theme &theme, const Visual &visual, Anchor side, const std::string &label, const FontStyle &style, float gap, const BraceOptions &options )
{
	const SideLine line = side_line( visual, side, gap );
	BraceOptions o = options;
	o.left = line.left;
	return make_labeled_brace( theme, line.from, line.to, label, style, o );
}
