#include "visuals/MorphLeaves.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>

namespace
{

float length_of( Point v )
{
	return std::sqrt( v.lengthSquared() );
}

struct State
{
	const std::vector<OutlineShape> &a;
	const std::vector<OutlineShape> &b;
	std::vector<bool> used_a;
	std::vector<bool> used_b;
	std::vector<LeafPair> pairs;

	State( const std::vector<OutlineShape> &shapes_a, const std::vector<OutlineShape> &shapes_b ) :
		a( shapes_a ),
		b( shapes_b ),
		used_a( shapes_a.size(), false ),
		used_b( shapes_b.size(), false )
	{
	}

	void pair( size_t i, size_t j )
	{
		used_a[i] = used_b[j] = true;
		pairs.push_back( { static_cast<int>( i ), static_cast<int>( j ) } );
	}

	std::vector<size_t> free_a() const
	{
		std::vector<size_t> out;
		for ( size_t i = 0; i < a.size(); ++i )
		{
			if ( !used_a[i] )
			{
				out.push_back( i );
			}
		}
		return out;
	}

	std::vector<size_t> free_b() const
	{
		std::vector<size_t> out;
		for ( size_t j = 0; j < b.size(); ++j )
		{
			if ( !used_b[j] )
			{
				out.push_back( j );
			}
		}
		return out;
	}
};

// Leaves under the same tag pair up in order. The most specific tags (the fewest leaves) go first, so a
// broad tag such as a whole row can't take leaves that a "numerator" would have paired exactly.
void match_by_tag( State &st )
{
	std::vector<std::string> names; // in order of first appearance in the first visual
	std::map<std::string, size_t> size;
	for ( const auto &shape: st.a )
	{
		for ( const auto &tag: shape.tags )
		{
			if ( size.emplace( tag, 0 ).second )
			{
				names.push_back( tag );
			}
			++size[tag];
		}
	}
	for ( const auto &shape: st.b )
	{
		for ( const auto &tag: shape.tags )
		{
			if ( auto it = size.find( tag ); it != size.end() )
			{
				++it->second;
			}
		}
	}
	std::stable_sort( names.begin(), names.end(), [&]( const auto &x, const auto &y ) { return size[x] < size[y]; } );

	auto has = []( const OutlineShape &shape, const std::string &tag )
	{
		return std::find( shape.tags.begin(), shape.tags.end(), tag ) != shape.tags.end();
	};

	for ( const auto &name: names )
	{
		std::vector<size_t> in_a;
		std::vector<size_t> in_b;
		for ( size_t i: st.free_a() )
		{
			if ( has( st.a[i], name ) )
			{
				in_a.push_back( i );
			}
		}
		for ( size_t j: st.free_b() )
		{
			if ( has( st.b[j], name ) )
			{
				in_b.push_back( j );
			}
		}
		for ( size_t k = 0; k < std::min( in_a.size(), in_b.size() ); ++k )
		{
			st.pair( in_a[k], in_b[k] );
		}
	}
}

// Identical leaves, kept in reading order (a longest common subsequence of their keys), so that
// x + 1 -> x + 2 keeps the x and the + and only changes the number
void match_by_glyph( State &st )
{
	const auto fa = st.free_a();
	const auto fb = st.free_b();
	const size_t n = fa.size();
	const size_t m = fb.size();

	std::vector<std::vector<size_t>> lcs( n + 1, std::vector<size_t>( m + 1, 0 ) );
	for ( size_t i = n; i-- > 0; )
	{
		for ( size_t j = m; j-- > 0; )
		{
			lcs[i][j] = st.a[fa[i]].key == st.b[fb[j]].key ? lcs[i + 1][j + 1] + 1 : std::max( lcs[i + 1][j], lcs[i][j + 1] );
		}
	}

	size_t i = 0;
	size_t j = 0;
	while ( i < n && j < m )
	{
		if ( st.a[fa[i]].key == st.b[fb[j]].key )
		{
			st.pair( fa[i], fb[j] );
			++i;
			++j;
		}
		else if ( lcs[i + 1][j] >= lcs[i][j + 1] )
		{
			++i;
		}
		else
		{
			++j;
		}
	}
}

// Where a leaf is and how big it is (the diagonal of its box)
struct Extent
{
	Point center;
	float diagonal = 0.F;
};

Extent extent_of( const OutlineShape &shape )
{
	AABB box;
	for ( const auto &c: shape.contours )
	{
		box.add( on_curve_bounds( c ) );
	}
	if ( box.empty() )
	{
		return {};
	}
	return { { ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F }, std::hypot( box.width(), box.height() ) };
}

// Leaves left over pair up only if they are near each other for their size and of a similar size. That
// lets a letter turn into the letter that takes its place, but sends unrelated parts (a parenthesis
// with nothing like it on the other side) off to fade rather than fly across as a blob. (Not applied to
// a visual that is a single leaf: see match_by_proximity.)
constexpr float MAX_SIZE_RATIO = 3.F;

bool plausible( const Extent &x, const Extent &y )
{
	const float small = std::min( x.diagonal, y.diagonal );
	const float big = std::max( x.diagonal, y.diagonal );
	if ( big > MAX_SIZE_RATIO * std::max( small, 1e-6F ) )
	{
		return false;
	}
	// within reach when the boxes are about touching
	return length_of( x.center - y.center ) <= 0.5F * ( x.diagonal + y.diagonal );
}

// Whatever is left: the closest plausible pairs first. When either visual is a single leaf the caller
// asked for that shape to turn into the other whatever the distance or size, so nothing is ruled out.
void match_by_proximity( State &st )
{
	const bool gated = st.a.size() > 1 && st.b.size() > 1;
	std::vector<Extent> ea;
	std::vector<Extent> eb;
	for ( const auto &shape: st.a )
	{
		ea.push_back( extent_of( shape ) );
	}
	for ( const auto &shape: st.b )
	{
		eb.push_back( extent_of( shape ) );
	}

	while ( true )
	{
		float best = std::numeric_limits<float>::max();
		size_t bi = 0;
		size_t bj = 0;
		bool found = false;
		for ( size_t i: st.free_a() )
		{
			for ( size_t j: st.free_b() )
			{
				if ( gated && !plausible( ea[i], eb[j] ) )
				{
					continue;
				}
				const float d = ( ea[i].center - eb[j].center ).lengthSquared();
				if ( d < best )
				{
					best = d;
					bi = i;
					bj = j;
					found = true;
				}
			}
		}
		if ( !found )
		{
			return;
		}
		st.pair( bi, bj );
	}
}

void match_by_index( State &st )
{
	for ( size_t i = 0; i < std::min( st.a.size(), st.b.size() ); ++i )
	{
		st.pair( i, i );
	}
}

}

std::vector<LeafPair> match_leaves( const std::vector<OutlineShape> &a, const std::vector<OutlineShape> &b, LeafMatch mode )
{
	State st( a, b );

	switch ( mode )
	{
		case LeafMatch::AUTO:
			match_by_tag( st );
			match_by_glyph( st );
			match_by_proximity( st );
			break;
		case LeafMatch::BY_TAG:
			match_by_tag( st );
			break;
		case LeafMatch::BY_GLYPH:
			match_by_glyph( st );
			break;
		case LeafMatch::BY_INDEX:
			match_by_index( st );
			break;
	}

	for ( size_t i: st.free_a() )
	{
		st.pairs.push_back( { static_cast<int>( i ), -1 } );
	}
	for ( size_t j: st.free_b() )
	{
		st.pairs.push_back( { -1, static_cast<int>( j ) } );
	}

	// Drawing order: by the second visual, with what only the first has kept just before its neighbour
	auto order = [&]( const LeafPair &p )
	{
		return p.b >= 0 ? static_cast<float>( p.b ) : static_cast<float>( p.a ) - 0.5F;
	};
	std::stable_sort( st.pairs.begin(), st.pairs.end(), [&]( const LeafPair &x, const LeafPair &y ) { return order( x ) < order( y ); } );
	return st.pairs;
}
