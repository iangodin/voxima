#include "visuals/MorphMatch.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numbers>

namespace
{

constexpr int ARC_STEPS = 16;

// -- small vector helpers (Point has no dot/length) -------------------------------------------------

float dot( Point a, Point b )
{
	return a.x() * b.x() + a.y() * b.y();
}

float length( Point v )
{
	return std::sqrt( v.lengthSquared() );
}

Point unit( Point v, Point fallback = { 1.F, 0.F } )
{
	const float len = length( v );
	return len > 1e-9F ? v / len : fallback;
}

Point lerp( Point a, Point b, float t )
{
	return a + ( b - a ) * t;
}

// -- Beziers ---------------------------------------------------------------------------------------

struct Bez
{
	Point p0, p1, p2, p3;
};

Point at( const Bez &b, float t )
{
	const float u = 1.F - t;
	return b.p0 * ( u * u * u ) + b.p1 * ( 3.F * u * u * t ) + b.p2 * ( 3.F * u * t * t ) + b.p3 * ( t * t * t );
}

void split( const Bez &b, float t, Bez &left, Bez &right )
{
	const Point p01 = lerp( b.p0, b.p1, t );
	const Point p12 = lerp( b.p1, b.p2, t );
	const Point p23 = lerp( b.p2, b.p3, t );
	const Point p012 = lerp( p01, p12, t );
	const Point p123 = lerp( p12, p23, t );
	const Point mid = lerp( p012, p123, t );
	left = { b.p0, p01, p012, mid };
	right = { mid, p123, p23, b.p3 };
}

// The part of the curve between t0 and t1 (t0 <= t1); a point if they meet
Bez sub( const Bez &b, float t0, float t1 )
{
	if ( t1 <= t0 )
	{
		const Point p = at( b, t0 );
		return { p, p, p, p };
	}
	if ( t0 <= 0.F && t1 >= 1.F )
	{
		return b;
	}
	Bez head, tail, unused;
	split( b, t1, head, unused );
	if ( t0 <= 0.F )
	{
		return head;
	}
	split( head, t0 / t1, unused, tail );
	return tail;
}

// Direction the curve leaves its start, skipping control points that sit on it
Point tangent_start( const Bez &b )
{
	for ( Point q: { b.p1, b.p2, b.p3 } )
	{
		if ( ( q - b.p0 ).lengthSquared() > 1e-12F )
		{
			return unit( q - b.p0 );
		}
	}
	return { 1.F, 0.F };
}

Point tangent_end( const Bez &b )
{
	for ( Point q: { b.p2, b.p1, b.p0 } )
	{
		if ( ( b.p3 - q ).lengthSquared() > 1e-12F )
		{
			return unit( b.p3 - q );
		}
	}
	return { 1.F, 0.F };
}

Point derivative( const Bez &b, float t )
{
	const float u = 1.F - t;
	return ( b.p1 - b.p0 ) * ( 3.F * u * u ) + ( b.p2 - b.p1 ) * ( 6.F * u * t ) + ( b.p3 - b.p2 ) * ( 3.F * t * t );
}

// t in (lo, hi) where one component of the derivative vanishes (a horizontal or vertical tangent)
void extrema( float d0, float d1, float d2, float lo, float hi, std::vector<float> &out )
{
	// derivative / 3 = a t^2 + b t + c
	const float a = d0 - 2.F * d1 + d2;
	const float b = 2.F * ( d1 - d0 );
	const float c = d0;
	auto add = [&]( float t )
	{
		if ( t > lo && t < hi )
		{
			out.push_back( t );
		}
	};
	if ( std::abs( a ) < 1e-9F )
	{
		if ( std::abs( b ) > 1e-9F )
		{
			add( -c / b );
		}
		return;
	}
	const float disc = b * b - 4.F * a * c;
	if ( disc < 0.F )
	{
		return;
	}
	const float root = std::sqrt( disc );
	add( ( -b - root ) / ( 2.F * a ) );
	add( ( -b + root ) / ( 2.F * a ) );
}

// -- Contour analysis --------------------------------------------------------------------------------

enum class Kind
{
	CORNER,
	EXTREMUM,
	SMOOTH
};

struct Position
{
	size_t seg = 0;
	float t = 0.F;
};

// A point where the contour could be matched with the other one
struct Knot
{
	Position pos;
	float s = 0.F; // arc-length fraction from the contour's start
	Point p;
	Point tangent; // direction of travel
	float turn = 0.F;
	Kind kind = Kind::SMOOTH;
	bool projected = false; // added to sit next to a knot of the other contour, not a feature of this one
};

struct Analysis
{
	std::vector<Bez> segs;
	std::vector<std::array<float, ARC_STEPS + 1>> table; // arc length from the segment start at t = k / ARC_STEPS
	std::vector<float> seg_start;
	float total = 0.F;
	std::vector<Knot> knots;
	Point centroid;
	float scale = 1.F;

	Analysis( const Contour &c, const AlignCosts &costs )
	{
		Point prev = c.start;
		for ( const auto &seg: c.segs )
		{
			segs.push_back( { prev, seg.c1, seg.c2, seg.to } );
			prev = seg.to;
		}

		AABB box;
		Point sum;
		size_t samples = 0;
		for ( const auto &b: segs )
		{
			auto &row = table.emplace_back();
			row[0] = 0.F;
			Point last = b.p0;
			for ( int k = 1; k <= ARC_STEPS; ++k )
			{
				const Point p = at( b, static_cast<float>( k ) / ARC_STEPS );
				row[k] = row[k - 1] + length( p - last );
				last = p;
				box.add( p );
				sum += p;
				++samples;
			}
			seg_start.push_back( total );
			total += row[ARC_STEPS];
		}
		centroid = samples ? sum / static_cast<float>( samples ) : c.start;
		scale = std::max( box.empty() ? 0.F : 0.5F * std::hypot( box.width(), box.height() ), 1e-6F );

		find_knots( costs );
	}

	float fraction( size_t seg, float t ) const
	{
		if ( total <= 0.F )
		{
			return 0.F;
		}
		const float pos = t * ARC_STEPS;
		const int k = std::min( static_cast<int>( pos ), ARC_STEPS - 1 );
		const float within = table[seg][k] + ( table[seg][k + 1] - table[seg][k] ) * ( pos - static_cast<float>( k ) );
		return ( seg_start[seg] + within ) / total;
	}

	// The point at arc-length fraction s (any real number; wrapped into [0, 1))
	Position locate( float s ) const
	{
		s -= std::floor( s );
		if ( total <= 0.F || segs.empty() )
		{
			return { 0, 0.F };
		}
		const float target = s * total;
		size_t seg = 0;
		while ( seg + 1 < segs.size() && seg_start[seg + 1] <= target )
		{
			++seg;
		}
		const float local = target - seg_start[seg];
		const auto &row = table[seg];
		int k = 0;
		while ( k < ARC_STEPS - 1 && row[k + 1] <= local )
		{
			++k;
		}
		const float span = row[k + 1] - row[k];
		const float f = span > 0.F ? std::clamp( ( local - row[k] ) / span, 0.F, 1.F ) : 0.F;
		return { seg, ( static_cast<float>( k ) + f ) / ARC_STEPS };
	}

	Point point( Position pos ) const
	{
		return at( segs[pos.seg], pos.t );
	}

	// Add a knot at the nearest point of this contour to each real knot of `other`, in this contour's
	// terms (both compared relative to their own centroid and size), with that knot's kind
	void add_projected( const Analysis &other, const AlignCosts &costs )
	{
		std::vector<Knot> added;
		for ( const Knot &source: other.knots )
		{
			if ( source.projected )
			{
				continue;
			}
			const Point target = centroid + ( source.p - other.centroid ) / other.scale * scale;
			const Position pos = nearest( target, source.tangent );
			const float s = fraction( pos.seg, pos.t );
			const Point p = at( segs[pos.seg], pos.t );

			bool crowded = false;
			for ( const auto *list: { &knots, &added } )
			{
				for ( const Knot &k: *list )
				{
					const float gap = std::abs( s - k.s );
					if ( std::min( gap, 1.F - gap ) < costs.min_projection_gap && length( p - k.p ) < 0.05F * scale )
					{
						crowded = true;
					}
				}
			}
			if ( crowded )
			{
				continue;
			}

			Knot k;
			k.pos = pos;
			k.s = s;
			k.p = p;
			k.tangent = unit( derivative( segs[pos.seg], pos.t ), source.tangent );
			k.turn = source.turn;
			k.kind = source.kind;
			k.projected = true;
			added.push_back( k );
		}

		knots.insert( knots.end(), added.begin(), added.end() );
		std::stable_sort(
			knots.begin(),
			knots.end(),
			[]( const Knot &a, const Knot &b )
			{
				return a.pos.seg < b.pos.seg || ( a.pos.seg == b.pos.seg && a.pos.t < b.pos.t );
			} );
	}

private:
	// The point of the contour nearest to q: sampled along every segment, then refined. Where the contour
	// passes the same place twice (a stroke made into a loop out and back lies on top of itself), the
	// pass going most nearly in direction `heading` wins: so the top of a circle goes to one pass and the bottom
	// (travelling the other way) to the other, instead of both to whichever comes first.
	Position nearest( Point q, Point heading ) const
	{
		constexpr int SAMPLES = 32;
		float best_d = std::numeric_limits<float>::max();
		for ( size_t i = 0; i < segs.size(); ++i )
		{
			for ( int k = 0; k <= SAMPLES; ++k )
			{
				best_d = std::min( best_d, ( at( segs[i], static_cast<float>( k ) / SAMPLES ) - q ).lengthSquared() );
			}
		}

		// Of the samples as near as that (to a hair, relative to the contour's size), the best heading
		const float near_enough = best_d + 1e-8F * scale * scale;
		Position best;
		float best_heading = -std::numeric_limits<float>::max();
		for ( size_t i = 0; i < segs.size(); ++i )
		{
			for ( int k = 0; k <= SAMPLES; ++k )
			{
				const float t = static_cast<float>( k ) / SAMPLES;
				if ( ( at( segs[i], t ) - q ).lengthSquared() <= near_enough )
				{
					const float h = dot( unit( derivative( segs[i], t ) ), heading );
					if ( h > best_heading )
					{
						best_heading = h;
						best = { i, t };
					}
				}
			}
		}
		// Narrow in on the minimum between the neighbouring samples
		float lo = std::max( 0.F, best.t - 1.F / SAMPLES );
		float hi = std::min( 1.F, best.t + 1.F / SAMPLES );
		for ( int it = 0; it < 20; ++it )
		{
			const float m1 = lo + ( hi - lo ) / 3.F;
			const float m2 = hi - ( hi - lo ) / 3.F;
			if ( ( at( segs[best.seg], m1 ) - q ).lengthSquared() < ( at( segs[best.seg], m2 ) - q ).lengthSquared() )
			{
				hi = m2;
			}
			else
			{
				lo = m1;
			}
		}
		best.t = ( lo + hi ) / 2.F;
		return best;
	}

	void find_knots( const AlignCosts &costs )
	{
		const size_t n = segs.size();
		for ( size_t i = 0; i < n; ++i )
		{
			const Bez &b = segs[i];

			Knot junction;
			junction.pos = { i, 0.F };
			junction.s = fraction( i, 0.F );
			junction.p = b.p0;
			junction.tangent = tangent_start( b );
			const Point incoming = tangent_end( segs[( i + n - 1 ) % n] );
			junction.turn = std::acos( std::clamp( dot( incoming, junction.tangent ), -1.F, 1.F ) );
			if ( junction.turn > costs.corner_angle )
			{
				junction.kind = Kind::CORNER;
			}
			else if ( std::abs( junction.tangent.x() ) < costs.axis_tolerance || std::abs( junction.tangent.y() ) < costs.axis_tolerance )
			{
				junction.kind = Kind::EXTREMUM;
			}
			knots.push_back( junction );

			// Extrema inside the segment
			std::vector<float> ts;
			extrema( b.p1.x() - b.p0.x(), b.p2.x() - b.p1.x(), b.p3.x() - b.p2.x(), 0.02F, 0.98F, ts );
			extrema( b.p1.y() - b.p0.y(), b.p2.y() - b.p1.y(), b.p3.y() - b.p2.y(), 0.02F, 0.98F, ts );
			std::sort( ts.begin(), ts.end() );
			for ( float t: ts )
			{
				Knot k;
				k.pos = { i, t };
				k.s = fraction( i, t );
				k.p = at( b, t );
				k.tangent = unit( derivative( b, t ), junction.tangent );
				k.kind = Kind::EXTREMUM;
				knots.push_back( k );
			}
		}
	}
};

// How much a knot deserves to anchor the alignment: corners first, then extrema
float strength( const Knot &k )
{
	switch ( k.kind )
	{
		case Kind::CORNER: return 1.F + k.turn;
		case Kind::EXTREMUM: return 0.5F;
		default: return 0.F;
	}
}

float kind_cost( Kind a, Kind b, const AlignCosts &c )
{
	if ( a == b )
	{
		switch ( a )
		{
			case Kind::CORNER: return c.corner_corner;
			case Kind::EXTREMUM: return c.extremum_extremum;
			default: return c.smooth_smooth;
		}
	}
	if ( a == Kind::SMOOTH ) std::swap( a, b );
	// a is now never SMOOTH unless both are
	if ( b == Kind::SMOOTH )
	{
		return a == Kind::CORNER ? c.corner_smooth : c.extremum_smooth;
	}
	return c.corner_extremum;
}

float skip_cost( const Knot &knot, const AlignCosts &c )
{
	if ( knot.projected )
	{
		return c.skip_projected;
	}
	switch ( knot.kind )
	{
		case Kind::CORNER: return c.skip_corner;
		case Kind::EXTREMUM: return c.skip_extremum;
		default: return c.skip_smooth;
	}
}

float wrap01( float s )
{
	return s - std::floor( s );
}

// -- Alignment ---------------------------------------------------------------------------------------

enum class Op : unsigned char
{
	MATCH,
	SKIP_A, // the knot of A has no partner on B; B is split to make one
	SKIP_B
};

struct Step
{
	Op op;
	size_t k; // index into A's knots, rotated to start at the anchor
	size_t l; // same for B
};

struct Alignment
{
	float cost = std::numeric_limits<float>::max();
	size_t anchor_a = 0;
	size_t anchor_b = 0;
	std::vector<Step> steps;
};

class Aligner
{
public:
	Aligner( const Analysis &a, const Analysis &b, const AlignCosts &costs ) :
		_a( a ),
		_b( b ),
		_costs( costs ),
		_na( a.knots.size() ),
		_nb( b.knots.size() )
	{
	}

	Alignment best() const
	{
		// Anchor on A's strongest knot (topmost among equals, so the result doesn't depend on where
		// the outline happens to start) and try it against every knot of B
		size_t anchor = 0;
		for ( size_t i = 1; i < _na; ++i )
		{
			const float si = strength( _a.knots[i] );
			const float sb = strength( _a.knots[anchor] );
			if ( si > sb + 1e-4F || ( std::abs( si - sb ) <= 1e-4F && _a.knots[i].p.y() > _a.knots[anchor].p.y() + 1e-6F ) )
			{
				anchor = i;
			}
		}

		// The knots of B to try: all of them, or the few nearest the anchor's place when there are many
		std::vector<size_t> tries( _nb );
		for ( size_t j = 0; j < _nb; ++j )
		{
			tries[j] = j;
		}
		if ( _nb > _costs.max_anchors )
		{
			const Point where = ( _a.knots[anchor].p - _a.centroid ) / _a.scale;
			auto distance = [&]( size_t j ) { return ( ( _b.knots[j].p - _b.centroid ) / _b.scale - where ).lengthSquared(); };
			std::partial_sort(
				tries.begin(),
				tries.begin() + static_cast<std::ptrdiff_t>( _costs.max_anchors ),
				tries.end(),
				[&]( size_t x, size_t y ) { return distance( x ) < distance( y ); } );
			tries.resize( _costs.max_anchors );
		}

		Alignment best;
		for ( size_t j: tries )
		{
			Alignment candidate = run( anchor, j );
			if ( candidate.cost < best.cost )
			{
				best = std::move( candidate );
			}
		}
		return best;
	}

	// Relative arc fraction from the anchor knot
	float rel_a( size_t anchor, size_t k ) const
	{
		return wrap01( _a.knots[( anchor + k ) % _na].s - _a.knots[anchor].s );
	}

	float rel_b( size_t anchor, size_t l ) const
	{
		return wrap01( _b.knots[( anchor + l ) % _nb].s - _b.knots[anchor].s );
	}

private:
	Alignment run( size_t anchor_a, size_t anchor_b ) const
	{
		const auto ka = [&]( size_t k ) -> const Knot & { return _a.knots[( anchor_a + k ) % _na]; };
		const auto kb = [&]( size_t l ) -> const Knot & { return _b.knots[( anchor_b + l ) % _nb]; };

		auto match_cost = [&]( size_t k, size_t l )
		{
			const Knot &x = ka( k );
			const Knot &y = kb( l );
			const float arc = std::abs( wrap01( x.s - _a.knots[anchor_a].s ) - wrap01( y.s - _b.knots[anchor_b].s ) );
			const float pos = length( ( x.p - _a.centroid ) / _a.scale - ( y.p - _b.centroid ) / _b.scale );
			const float tan = std::acos( std::clamp( dot( x.tangent, y.tangent ), -1.F, 1.F ) ) / std::numbers::pi_v<float>;
			return _costs.w_arc * arc + _costs.w_position * pos + _costs.w_tangent * tan + kind_cost( x.kind, y.kind, _costs );
		};

		const size_t rows = _na;
		const size_t cols = _nb;
		std::vector<float> cost( rows * cols, 0.F );
		std::vector<Op> from( rows * cols, Op::MATCH );
		auto idx = [&]( size_t k, size_t l ) { return k * cols + l; };

		cost[idx( 0, 0 )] = match_cost( 0, 0 );
		for ( size_t k = 0; k < rows; ++k )
		{
			for ( size_t l = 0; l < cols; ++l )
			{
				if ( k == 0 && l == 0 )
				{
					continue;
				}
				float best = std::numeric_limits<float>::max();
				Op op = Op::MATCH;
				if ( k > 0 && l > 0 )
				{
					best = cost[idx( k - 1, l - 1 )] + match_cost( k, l );
				}
				if ( k > 0 )
				{
					const float c = cost[idx( k - 1, l )] + skip_cost( ka( k ), _costs );
					if ( c < best )
					{
						best = c;
						op = Op::SKIP_A;
					}
				}
				if ( l > 0 )
				{
					const float c = cost[idx( k, l - 1 )] + skip_cost( kb( l ), _costs );
					if ( c < best )
					{
						best = c;
						op = Op::SKIP_B;
					}
				}
				cost[idx( k, l )] = best;
				from[idx( k, l )] = op;
			}
		}

		Alignment result;
		result.cost = cost[idx( rows - 1, cols - 1 )];
		result.anchor_a = anchor_a;
		result.anchor_b = anchor_b;

		size_t k = rows - 1;
		size_t l = cols - 1;
		while ( k > 0 || l > 0 )
		{
			const Op op = from[idx( k, l )];
			result.steps.push_back( { op, k, l } );
			if ( op == Op::MATCH )
			{
				--k;
				--l;
			}
			else if ( op == Op::SKIP_A )
			{
				--k;
			}
			else
			{
				--l;
			}
		}
		result.steps.push_back( { Op::MATCH, 0, 0 } );
		std::reverse( result.steps.begin(), result.steps.end() );
		return result;
	}

	const Analysis &_a;
	const Analysis &_b;
	AlignCosts _costs;
	size_t _na;
	size_t _nb;
};

// A place to cut a contour. Knots are cut where they are (exact); the others are placed by arc fraction,
// which can land a hair before or after the knot they belong next to.
struct Cut
{
	Position pos;
	bool exact = false;
};

// Rebuild a contour cut at the given places, in order around the loop starting from cuts[0]
Contour cut_contour( const Analysis &an, const std::vector<Cut> &cuts )
{
	const size_t count = an.segs.size();
	const size_t n = cuts.size();
	const Position anchor = cuts[0].pos;

	// A position counted from the anchor: which segment (0 is the anchor's own) and how far along it.
	// The anchor's segment before the anchor point is the very end of the loop, a whole turn later.
	struct Keyed
	{
		size_t seg;
		float t;
	};
	auto key = [&]( Position p ) -> Keyed
	{
		if ( p.seg == anchor.seg && p.t < anchor.t - 1e-6F )
		{
			return { count, p.t };
		}
		return { ( p.seg + count - anchor.seg ) % count, p.t };
	};
	auto before = []( const Keyed &a, const Keyed &b )
	{
		return a.seg < b.seg || ( a.seg == b.seg && a.t < b.t - 1e-6F );
	};

	std::vector<Keyed> keys;
	for ( const auto &cut: cuts )
	{
		keys.push_back( key( cut.pos ) );
	}

	// Rounding can put a placed cut just before the knot it follows. Cuts must run forward, so pull
	// the one that is not exact into line (an exact knot never moves).
	for ( size_t pass = 0; pass < n; ++pass )
	{
		bool changed = false;
		for ( size_t m = 1; m < n; ++m )
		{
			if ( before( keys[m], keys[m - 1] ) )
			{
				if ( !cuts[m].exact || cuts[m - 1].exact )
				{
					keys[m] = keys[m - 1];
				}
				else
				{
					keys[m - 1] = keys[m];
				}
				changed = true;
			}
		}
		if ( !changed )
		{
			break;
		}
	}

	Contour out;
	out.start = an.point( anchor );
	for ( size_t m = 0; m < n; ++m )
	{
		const Keyed from = keys[m];
		const Keyed to = m + 1 < n ? keys[m + 1] : Keyed{ count, anchor.t };
		const size_t seg = ( anchor.seg + from.seg ) % count;

		// Every junction of the contour is a cut, so the way to the next cut never crosses one: it stays
		// inside a segment, or runs to the end of it (the next cut is the junction).
		const Bez piece = from.seg == to.seg ? sub( an.segs[seg], from.t, std::max( from.t, to.t ) ) : sub( an.segs[seg], from.t, 1.F );
		out.segs.push_back( { piece.p1, piece.p2, m + 1 == n ? out.start : piece.p3 } );
	}
	return out;
}

// -- Contour features for pairing ---------------------------------------------------------------------

struct Feature
{
	double area = 0.0; // signed
	Point centroid;
};

Feature feature_of( const Contour &c )
{
	Analysis an( c, {} );
	return { signed_area( c ), an.centroid };
}

}

ContourPair align_contours( const Contour &a, const Contour &b, const AlignCosts &costs )
{
	if ( a.segs.empty() || b.segs.empty() )
	{
		return { a, b };
	}

	Analysis an( a, costs );
	Analysis bn( b, costs );
	{
		const Analysis real_a = an;
		const Analysis real_b = bn;
		an.add_projected( real_b, costs );
		bn.add_projected( real_a, costs );
	}
	const Aligner aligner( an, bn, costs );
	const Alignment al = aligner.best();

	const size_t na = an.knots.size();
	const size_t nb = bn.knots.size();
	const float anchor_sa = an.knots[al.anchor_a].s;
	const float anchor_sb = bn.knots[al.anchor_b].s;

	std::vector<Step> steps = al.steps;
	const size_t count = steps.size();

	// A step past the last one stands for the anchor again, a full turn later
	auto rel_a = [&]( size_t step ) { return step >= count ? 1.F : aligner.rel_a( al.anchor_a, steps[step].k ); };
	auto rel_b = [&]( size_t step ) { return step >= count ? 1.F : aligner.rel_b( al.anchor_b, steps[step].l ); };

	// Between two matches the alignment lists skipped knots of A and of B in no particular order, but
	// both contours must be cut in order around the loop. Put each stretch in order of how far along it
	// each knot sits (as a fraction of its own side's stretch).
	{
		std::vector<size_t> matches;
		for ( size_t i = 0; i < count; ++i )
		{
			if ( steps[i].op == Op::MATCH )
			{
				matches.push_back( i );
			}
		}
		for ( size_t j = 0; j < matches.size(); ++j )
		{
			const size_t before = matches[j];
			const size_t after = j + 1 < matches.size() ? matches[j + 1] : count;
			auto progress = [&]( size_t i )
			{
				const bool on_a = steps[i].op == Op::SKIP_A;
				const float from = on_a ? rel_a( before ) : rel_b( before );
				const float span = ( on_a ? rel_a( after ) : rel_b( after ) ) - from;
				return span > 1e-9F ? ( ( on_a ? rel_a( i ) : rel_b( i ) ) - from ) / span : 0.F;
			};
			std::vector<std::pair<float, Step>> gap;
			for ( size_t i = before + 1; i < after; ++i )
			{
				gap.emplace_back( progress( i ), steps[i] );
			}
			std::stable_sort( gap.begin(), gap.end(), []( const auto &x, const auto &y ) { return x.first < y.first; } );
			for ( size_t i = before + 1; i < after; ++i )
			{
				steps[i] = gap[i - before - 1].second;
			}
		}
	}

	// For each step, the nearest matched steps before and after it
	std::vector<size_t> prev_match( count, 0 );
	std::vector<size_t> next_match( count, count );
	for ( size_t i = 1; i < count; ++i )
	{
		prev_match[i] = steps[i - 1].op == Op::MATCH ? i - 1 : prev_match[i - 1];
	}
	for ( size_t i = count; i-- > 0; )
	{
		if ( i + 1 < count )
		{
			next_match[i] = steps[i + 1].op == Op::MATCH ? i + 1 : next_match[i + 1];
		}
	}

	std::vector<Cut> cut_a;
	std::vector<Cut> cut_b;
	for ( size_t i = 0; i < count; ++i )
	{
		const Step &st = steps[i];
		const size_t before = prev_match[i];
		const size_t after = next_match[i];

		if ( st.op == Op::MATCH )
		{
			const Knot &ka = an.knots[( al.anchor_a + st.k ) % na];
			const Knot &kb = bn.knots[( al.anchor_b + st.l ) % nb];
			cut_a.push_back( { ka.pos, true } );
			cut_b.push_back( { kb.pos, true } );
		}
		else if ( st.op == Op::SKIP_A )
		{
			const Knot &ka = an.knots[( al.anchor_a + st.k ) % na];
			// Where B has to be split: as far along between its matched neighbours as A's knot is
			const float span = rel_a( after ) - rel_a( before );
			const float f = span > 1e-9F ? ( rel_a( i ) - rel_a( before ) ) / span : 0.F;
			const float rb = rel_b( before ) + f * ( rel_b( after ) - rel_b( before ) );
			cut_a.push_back( { ka.pos, true } );
			cut_b.push_back( { bn.locate( anchor_sb + rb ), false } );
		}
		else
		{
			const Knot &kb = bn.knots[( al.anchor_b + st.l ) % nb];
			const float span = rel_b( after ) - rel_b( before );
			const float f = span > 1e-9F ? ( rel_b( i ) - rel_b( before ) ) / span : 0.F;
			const float ra = rel_a( before ) + f * ( rel_a( after ) - rel_a( before ) );
			cut_a.push_back( { an.locate( anchor_sa + ra ), false } );
			cut_b.push_back( { kb.pos, true } );
		}
	}

	return { cut_contour( an, cut_a ), cut_contour( bn, cut_b ) };
}

ContourPair align_open( const Contour &a, const Contour &b )
{
	if ( a.segs.empty() || b.segs.empty() )
	{
		return { a, b };
	}

	const Analysis an( a, {} );
	const Analysis bn( b, {} );

	// Where each contour has a joint, as how far along it is: those are where both get cut
	struct Joint
	{
		float s;
		bool in_a;
		bool in_b;
		size_t seg_a;
		size_t seg_b;
	};
	std::vector<Joint> joints;
	for ( size_t i = 1; i < an.segs.size(); ++i )
	{
		joints.push_back( { an.fraction( i, 0.F ), true, false, i, 0 } );
	}
	for ( size_t j = 1; j < bn.segs.size(); ++j )
	{
		joints.push_back( { bn.fraction( j, 0.F ), false, true, 0, j } );
	}
	std::stable_sort( joints.begin(), joints.end(), []( const Joint &x, const Joint &y ) { return x.s < y.s; } );

	// Joints of the two that fall in the same place are one
	std::vector<Joint> merged;
	for ( const Joint &joint: joints )
	{
		if ( !merged.empty() && joint.s - merged.back().s < 1e-4F && ( ( joint.in_a && !merged.back().in_a ) || ( joint.in_b && !merged.back().in_b ) ) )
		{
			Joint &last = merged.back();
			if ( joint.in_a )
			{
				last.in_a = true;
				last.seg_a = joint.seg_a;
			}
			else
			{
				last.in_b = true;
				last.seg_b = joint.seg_b;
			}
		}
		else
		{
			merged.push_back( joint );
		}
	}

	std::vector<Position> cut_a{ { 0, 0.F } };
	std::vector<Position> cut_b{ { 0, 0.F } };
	for ( const Joint &joint: merged )
	{
		cut_a.push_back( joint.in_a ? Position{ joint.seg_a, 0.F } : an.locate( joint.s ) );
		cut_b.push_back( joint.in_b ? Position{ joint.seg_b, 0.F } : bn.locate( joint.s ) );
	}
	cut_a.push_back( { an.segs.size() - 1, 1.F } );
	cut_b.push_back( { bn.segs.size() - 1, 1.F } );

	auto build = [&]( const Analysis &analysis, const std::vector<Position> &cuts )
	{
		Contour out;
		out.closed = false;
		out.start = analysis.point( cuts.front() );
		for ( size_t m = 0; m + 1 < cuts.size(); ++m )
		{
			const Position from = cuts[m];
			const Position to = cuts[m + 1];
			// No joint is ever inside a piece, so it stays in one segment or runs to the end of it
			const Bez piece = from.seg == to.seg ? sub( analysis.segs[from.seg], from.t, std::max( from.t, to.t ) )
												 : sub( analysis.segs[from.seg], from.t, 1.F );
			out.segs.push_back( { piece.p1, piece.p2, piece.p3 } );
		}
		return out;
	};

	return { build( an, cut_a ), build( bn, cut_b ) };
}

Contour point_contour( Point p, size_t segments )
{
	Contour c;
	c.start = p;
	c.segs.assign( segments, CubicSeg{ p, p, p } );
	return c;
}

std::vector<ContourPair> match_contours( const OutlineShape &a, const OutlineShape &b, const AlignCosts &costs )
{
	struct Item
	{
		Contour contour;
		Feature feature;
		bool outer = true;
		bool paired = false;
	};

	auto describe = [&]( const std::vector<Contour> &contours )
	{
		std::vector<Item> items;
		for ( const auto &c: contours )
		{
			items.push_back( { c, feature_of( c ), true, false } );
		}
		// Winding of the biggest contour is the outer one; the other winding is holes
		double biggest = 0.0;
		double sign = 1.0;
		for ( const auto &item: items )
		{
			if ( std::abs( item.feature.area ) > biggest )
			{
				biggest = std::abs( item.feature.area );
				sign = item.feature.area < 0.0 ? -1.0 : 1.0;
			}
		}
		for ( auto &item: items )
		{
			item.outer = item.feature.area * sign >= 0.0;
		}
		return std::pair{ std::move( items ), sign };
	};

	// Strokes go with strokes; a stroke with none to go with becomes a loop that goes out and back, so it can
	// turn into (or out of) a filled shape
	std::vector<Contour> closed_a;
	std::vector<Contour> closed_b;
	std::vector<Contour> open_a;
	std::vector<Contour> open_b;
	for ( const auto &c: a.contours )
	{
		( c.closed ? closed_a : open_a ).push_back( c );
	}
	for ( const auto &c: b.contours )
	{
		( c.closed ? closed_b : open_b ).push_back( c );
	}

	std::vector<ContourPair> result;
	while ( !open_a.empty() && !open_b.empty() )
	{
		float best = std::numeric_limits<float>::max();
		size_t bi = 0;
		size_t bj = 0;
		for ( size_t i = 0; i < open_a.size(); ++i )
		{
			for ( size_t j = 0; j < open_b.size(); ++j )
			{
				const float d = ( feature_of( open_a[i] ).centroid - feature_of( open_b[j] ).centroid ).lengthSquared();
				if ( d < best )
				{
					best = d;
					bi = i;
					bj = j;
				}
			}
		}
		result.push_back( align_open( open_a[bi], open_b[bj] ) );
		open_a.erase( open_a.begin() + static_cast<std::ptrdiff_t>( bi ) );
		open_b.erase( open_b.begin() + static_cast<std::ptrdiff_t>( bj ) );
	}
	for ( const auto &c: open_a )
	{
		closed_a.push_back( closed_loop( c ) );
	}
	for ( const auto &c: open_b )
	{
		closed_b.push_back( closed_loop( c ) );
	}

	auto [items_a, sign_a] = describe( closed_a );
	auto [items_b, sign_b] = describe( closed_b );

	// Same winding on both sides, or the fill rule would turn holes into fill mid-morph
	if ( sign_a != sign_b )
	{
		for ( auto &item: items_b )
		{
			item.contour = reversed( item.contour );
			item.feature.area = -item.feature.area;
		}
	}

	// Compare positions relative to each shape, so translation and scale don't matter
	auto frame_of = [&]( const std::vector<Item> &items )
	{
		AABB box;
		for ( const auto &item: items )
		{
			box.add( on_curve_bounds( item.contour ) );
		}
		const Point center = box.empty() ? Point{} : Point{ ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F };
		const float scale = box.empty() ? 1.F : std::max( 0.5F * std::hypot( box.width(), box.height() ), 1e-6F );
		return std::pair{ center, scale };
	};
	const auto [center_a, scale_a] = frame_of( items_a );
	const auto [center_b, scale_b] = frame_of( items_b );

	auto pair_cost = [&]( const Item &x, const Item &y )
	{
		const float d = length( ( x.feature.centroid - center_a ) / scale_a - ( y.feature.centroid - center_b ) / scale_b );
		const double ax = std::abs( x.feature.area ) + 1e-9;
		const double ay = std::abs( y.feature.area ) + 1e-9;
		return d + 0.3F * static_cast<float>( std::abs( std::log( ax / ay ) ) );
	};

	// Best pairs first, holes with holes and outers with outers
	for ( bool outer: { true, false } )
	{
		while ( true )
		{
			float best = std::numeric_limits<float>::max();
			Item *pa = nullptr;
			Item *pb = nullptr;
			for ( auto &x: items_a )
			{
				for ( auto &y: items_b )
				{
					if ( x.paired || y.paired || x.outer != outer || y.outer != outer )
					{
						continue;
					}
					const float c = pair_cost( x, y );
					if ( c < best )
					{
						best = c;
						pa = &x;
						pb = &y;
					}
				}
			}
			if ( !pa )
			{
				break;
			}
			pa->paired = pb->paired = true;
			result.push_back( align_contours( pa->contour, pb->contour, costs ) );
		}
	}

	// Contours with no partner shrink into (or grow out of) the nearest contour of the other shape
	auto nearest = [&]( const Item &x, const std::vector<Item> &others, Point center, float scale, Point own_center, float own_scale )
	{
		Point target = x.feature.centroid;
		float best = std::numeric_limits<float>::max();
		for ( const auto &o: others )
		{
			const float d = length( ( x.feature.centroid - own_center ) / own_scale - ( o.feature.centroid - center ) / scale );
			if ( d < best )
			{
				best = d;
				target = o.feature.centroid;
			}
		}
		return target;
	};
	for ( const auto &x: items_a )
	{
		if ( !x.paired )
		{
			const Point target = nearest( x, items_b, center_b, scale_b, center_a, scale_a );
			result.push_back( { x.contour, point_contour( target, x.contour.segs.size() ) } );
		}
	}
	for ( const auto &y: items_b )
	{
		if ( !y.paired )
		{
			const Point target = nearest( y, items_a, center_a, scale_a, center_b, scale_b );
			result.push_back( { point_contour( target, y.contour.segs.size() ), y.contour } );
		}
	}

	return result;
}
