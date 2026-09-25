#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <visuals/CreateText.h>
#include <math/Expr.h>
#include <themes/NewComputerModern.h>
#include <visuals/Morph.h>
#include <visuals/MorphLeaves.h>
#include <visuals/Outline.h>
#include <visuals/Tags.h>

#include <algorithm>

using Catch::Approx;
using Pairs = std::vector<LeafPair>;

namespace
{
// A leaf that is just a tiny square at (x, y), with a key and tags
OutlineShape leaf( uint64_t key, float x, float y = 0.F, std::vector<std::string> tags = {} )
{
	OutlineShape shape;
	Contour c;
	c.start = { x, y };
	const Point corners[] = { { x + 1.F, y }, { x + 1.F, y + 1.F }, { x, y + 1.F }, { x, y } };
	Point prev = c.start;
	for ( auto p: corners )
	{
		c.segs.push_back( { prev, p, p } );
		prev = p;
	}
	shape.contours.push_back( c );
	shape.key = key;
	shape.tags = std::move( tags );
	return shape;
}

bool has( const Pairs &pairs, int a, int b )
{
	return std::find( pairs.begin(), pairs.end(), LeafPair{ a, b } ) != pairs.end();
}

// Every leaf on either side appears exactly once
bool complete( const Pairs &pairs, size_t na, size_t nb )
{
	std::vector<int> seen_a( na, 0 );
	std::vector<int> seen_b( nb, 0 );
	for ( auto p: pairs )
	{
		if ( p.a >= 0 ) ++seen_a[static_cast<size_t>( p.a )];
		if ( p.b >= 0 ) ++seen_b[static_cast<size_t>( p.b )];
		if ( p.a < 0 && p.b < 0 ) return false;
	}
	return std::all_of( seen_a.begin(), seen_a.end(), []( int n ) { return n == 1; } ) &&
		   std::all_of( seen_b.begin(), seen_b.end(), []( int n ) { return n == 1; } );
}
}

TEST_CASE( "match_leaves BY_INDEX pairs the i-th with the i-th", "[morph]" )
{
	std::vector<OutlineShape> a{ leaf( 1, 0 ), leaf( 2, 1 ), leaf( 3, 2 ) };
	std::vector<OutlineShape> b{ leaf( 9, 0 ), leaf( 8, 1 ) };

	const auto pairs = match_leaves( a, b, LeafMatch::BY_INDEX );
	CHECK( complete( pairs, 3, 2 ) );
	CHECK( has( pairs, 0, 0 ) );
	CHECK( has( pairs, 1, 1 ) );
	CHECK( has( pairs, 2, -1 ) ); // the extra one fades out
}

TEST_CASE( "match_leaves BY_TAG pairs leaves under the same tag", "[morph]" )
{
	std::vector<OutlineShape> a{ leaf( 1, 0, 0, { "numerator" } ), leaf( 2, 5, 0, { "denominator" } ) };
	std::vector<OutlineShape> b{ leaf( 3, 0, 0, { "denominator" } ), leaf( 4, 5, 0, { "numerator" } ) };

	const auto pairs = match_leaves( a, b, LeafMatch::BY_TAG );
	CHECK( complete( pairs, 2, 2 ) );
	CHECK( has( pairs, 0, 1 ) );
	CHECK( has( pairs, 1, 0 ) );
}

TEST_CASE( "match_leaves BY_TAG leaves untagged leaves alone", "[morph]" )
{
	std::vector<OutlineShape> a{ leaf( 1, 0, 0, { "x" } ), leaf( 2, 5 ) };
	std::vector<OutlineShape> b{ leaf( 1, 0, 0, { "x" } ), leaf( 2, 5 ) };

	const auto pairs = match_leaves( a, b, LeafMatch::BY_TAG );
	CHECK( complete( pairs, 2, 2 ) );
	CHECK( has( pairs, 0, 0 ) );
	CHECK( has( pairs, 1, -1 ) );
	CHECK( has( pairs, -1, 1 ) );
}

TEST_CASE( "match_leaves takes the most specific tag first", "[morph]" )
{
	// "row" is on everything; "numerator" on one leaf each. The numerators must pair with each other
	// even though they sit at different places in the row.
	std::vector<OutlineShape> a{ leaf( 1, 0, 0, { "row", "numerator" } ), leaf( 2, 5, 0, { "row" } ) };
	std::vector<OutlineShape> b{ leaf( 3, 0, 0, { "row" } ), leaf( 4, 5, 0, { "row", "numerator" } ) };

	const auto pairs = match_leaves( a, b, LeafMatch::BY_TAG );
	CHECK( has( pairs, 0, 1 ) );
	CHECK( has( pairs, 1, 0 ) );
}

TEST_CASE( "match_leaves BY_GLYPH keeps identical leaves in reading order", "[morph]" )
{
	std::vector<OutlineShape> a{ leaf( 1, 0 ), leaf( 2, 1 ), leaf( 3, 2 ), leaf( 4, 3 ) };
	std::vector<OutlineShape> b{ leaf( 1, 0 ), leaf( 9, 1 ), leaf( 3, 2 ), leaf( 4, 3 ) };

	const auto pairs = match_leaves( a, b, LeafMatch::BY_GLYPH );
	CHECK( complete( pairs, 4, 4 ) );
	CHECK( has( pairs, 0, 0 ) );
	CHECK( has( pairs, 2, 2 ) );
	CHECK( has( pairs, 3, 3 ) );
	// the one that differs has no identical partner
	CHECK( has( pairs, 1, -1 ) );
	CHECK( has( pairs, -1, 1 ) );
}

TEST_CASE( "match_leaves BY_GLYPH does not cross pairs", "[morph]" )
{
	// two of the same letter, in a different arrangement: order is kept, not distance
	std::vector<OutlineShape> a{ leaf( 5, 0 ), leaf( 6, 1 ), leaf( 5, 2 ) };
	std::vector<OutlineShape> b{ leaf( 6, 0 ), leaf( 5, 1 ) };

	const auto pairs = match_leaves( a, b, LeafMatch::BY_GLYPH );
	CHECK( complete( pairs, 3, 2 ) );
	// longest common subsequence has length 1; whichever it picks, no two pairs may cross
	int last_b = -1;
	for ( auto p: pairs )
	{
		if ( p.a >= 0 && p.b >= 0 )
		{
			CHECK( p.b > last_b );
			last_b = p.b;
		}
	}
}

TEST_CASE( "match_leaves AUTO: tags, then glyphs, then the closest", "[morph]" )
{
	// a, b: tagged pair. c: identical glyph. d, e: neither, so they pair by distance.
	std::vector<OutlineShape> a{
		leaf( 1, 0, 0, { "power" } ), leaf( 2, 3 ), leaf( 3, 10 ), leaf( 4, 20 ) };
	std::vector<OutlineShape> b{
		leaf( 7, 20, 0, { "power" } ), leaf( 2, 3 ), leaf( 8, 11 ), leaf( 9, 21 ) };

	const auto pairs = match_leaves( a, b, LeafMatch::AUTO );
	CHECK( complete( pairs, 4, 4 ) );
	CHECK( has( pairs, 0, 0 ) ); // by tag, though far apart
	CHECK( has( pairs, 1, 1 ) ); // identical
	CHECK( has( pairs, 2, 2 ) ); // 10 -> 11
	CHECK( has( pairs, 3, 3 ) ); // 20 -> 21
}

TEST_CASE( "match_leaves proximity pairs the closest first", "[morph]" )
{
	std::vector<OutlineShape> a{ leaf( 1, 0 ), leaf( 2, 10 ) };
	std::vector<OutlineShape> b{ leaf( 3, 9 ) };

	const auto pairs = match_leaves( a, b, LeafMatch::AUTO );
	CHECK( complete( pairs, 2, 1 ) );
	CHECK( has( pairs, 1, 0 ) );
	CHECK( has( pairs, 0, -1 ) );
}

TEST_CASE( "match_leaves proximity leaves unrelated parts to fade", "[morph]" )
{
	SECTION( "a single shape turns into a single shape, however far or different" )
	{
		std::vector<OutlineShape> a{ leaf( 1, 0 ) };
		std::vector<OutlineShape> b{ leaf( 2, 50 ) };
		CHECK( has( match_leaves( a, b, LeafMatch::AUTO ), 0, 0 ) );
	}

	SECTION( "leaves far apart for their size do not pair" )
	{
		std::vector<OutlineShape> a{ leaf( 1, 0 ), leaf( 3, 100 ) };
		std::vector<OutlineShape> b{ leaf( 2, 50 ), leaf( 4, 150 ) };
		const auto pairs = match_leaves( a, b, LeafMatch::AUTO );
		CHECK( complete( pairs, 2, 2 ) );
		CHECK( has( pairs, 0, -1 ) );
		CHECK( has( pairs, -1, 0 ) );
	}

	SECTION( "leaves of very different size do not pair, even side by side" )
	{
		std::vector<OutlineShape> a{ leaf( 1, 0 ), leaf( 5, 0, 20 ) };
		OutlineShape big = leaf( 2, 0 );
		for ( auto &seg: big.contours[0].segs )
		{
			seg.c1 = seg.c1 * 10.F;
			seg.c2 = seg.c2 * 10.F;
			seg.to = seg.to * 10.F;
		}
		big.contours[0].start = big.contours[0].start * 10.F;
		std::vector<OutlineShape> b{ big, leaf( 6, 0, 20 ) };
		const auto pairs = match_leaves( a, b, LeafMatch::AUTO );
		CHECK( has( pairs, 0, -1 ) );
		CHECK( has( pairs, -1, 0 ) );
	}

	SECTION( "leaves in the same place and size do pair" )
	{
		std::vector<OutlineShape> a{ leaf( 1, 0 ), leaf( 3, 20 ) };
		std::vector<OutlineShape> b{ leaf( 2, 0.5F ), leaf( 4, 20.5F ) };
		CHECK( has( match_leaves( a, b, LeafMatch::AUTO ), 0, 0 ) );
	}
}

TEST_CASE( "match_leaves lets parts that only one side names fade", "[morph]" )
{
	// Both leaf pairs sit in the same place at the same size, but the first leaf of `a` is tagged "open",
	// which nothing in `b` is: it has nothing to become, so it fades even though something is next to it.
	std::vector<OutlineShape> a{ leaf( 1, 0, 0, { "open" } ), leaf( 3, 20 ) };
	std::vector<OutlineShape> b{ leaf( 2, 0.5F ), leaf( 4, 20.5F ) };

	const auto pairs = match_leaves( a, b, LeafMatch::AUTO );
	CHECK( complete( pairs, 2, 2 ) );
	CHECK( has( pairs, 0, -1 ) );
	CHECK( has( pairs, -1, 0 ) );
	CHECK( has( pairs, 1, 1 ) );

	SECTION( "unless the other side has that name too" )
	{
		b[0].tags = { "open" };
		CHECK( has( match_leaves( a, b, LeafMatch::AUTO ), 0, 0 ) );
	}

	SECTION( "a single shape is never held back by its tag" )
	{
		std::vector<OutlineShape> one_a{ leaf( 1, 0, 0, { "circle" } ) };
		std::vector<OutlineShape> one_b{ leaf( 2, 30 ) };
		CHECK( has( match_leaves( one_a, one_b, LeafMatch::AUTO ), 0, 0 ) );
	}
}

TEST_CASE( "match_leaves lists leaves in the second visual's order", "[morph]" )
{
	std::vector<OutlineShape> a{ leaf( 1, 0 ), leaf( 2, 1 ), leaf( 3, 2 ) };
	std::vector<OutlineShape> b{ leaf( 3, 0 ), leaf( 2, 1 ), leaf( 1, 2 ) };

	const auto pairs = match_leaves( a, b, LeafMatch::BY_GLYPH );
	int last = -1;
	for ( auto p: pairs )
	{
		if ( p.b >= 0 )
		{
			CHECK( p.b > last );
			last = p.b;
		}
	}
}

TEST_CASE( "match_leaves handles empty visuals", "[morph]" )
{
	std::vector<OutlineShape> none;
	std::vector<OutlineShape> some{ leaf( 1, 0 ) };

	CHECK( match_leaves( none, none ).empty() );
	CHECK( match_leaves( some, none ) == Pairs{ { 0, -1 } } );
	CHECK( match_leaves( none, some ) == Pairs{ { -1, 0 } } );
}

TEST_CASE( "to_outline gives leaves their tags and a geometry key", "[morph]" )
{
	Theme theme = NewComputerModern::create_theme();
	math::Context context( theme );
	using namespace math;

	auto built = fraction( text( "1" ), text( "2" ) ).build( context );
	const auto shapes = to_outline( *built );
	REQUIRE( shapes.size() == 3 ); // numerator glyph, bar, denominator glyph

	auto tagged = [&]( const std::string &name )
	{
		for ( const auto &shape: shapes )
		{
			if ( std::find( shape.tags.begin(), shape.tags.end(), name ) != shape.tags.end() )
			{
				return true;
			}
		}
		return false;
	};
	CHECK( tagged( "numerator" ) );
	CHECK( tagged( "denominator" ) );
	CHECK( tagged( "bar" ) );

	for ( const auto &shape: shapes )
	{
		CHECK( shape.key != 0 );
	}

	SECTION( "the same glyph built twice has the same key, a different one does not" )
	{
		auto again = fraction( text( "1" ), text( "3" ) ).build( context );
		const auto other = to_outline( *again );
		REQUIRE( other.size() == 3 );
		auto numerator = []( const std::vector<OutlineShape> &list ) -> const OutlineShape &
		{
			return *std::find_if(
				list.begin(),
				list.end(),
				[]( const OutlineShape &s ) { return std::find( s.tags.begin(), s.tags.end(), "numerator" ) != s.tags.end(); } );
		};
		auto denominator = []( const std::vector<OutlineShape> &list ) -> const OutlineShape &
		{
			return *std::find_if(
				list.begin(),
				list.end(),
				[]( const OutlineShape &s ) { return std::find( s.tags.begin(), s.tags.end(), "denominator" ) != s.tags.end(); } );
		};
		CHECK( numerator( shapes ).key == numerator( other ).key );
		CHECK( denominator( shapes ).key != denominator( other ).key );
	}
}

TEST_CASE( "Morph of a formula keeps what did not change", "[morph]" )
{
	Theme theme = NewComputerModern::create_theme();
	math::Context context( theme );
	using namespace math;

	// x^2 + 1/2  ->  x^3 + 3/4
	auto before = row( { ord( superscript( text( "x" ), text( "2" ) ) ),
						 bin( "+" ),
						 inner( fraction( text( "1" ), text( "2" ) ) ) } ).build( context );
	auto after = row( { ord( superscript( text( "x" ), text( "3" ) ) ),
						bin( "+" ),
						inner( fraction( text( "3" ), text( "4" ) ) ) } ).build( context );

	const auto shapes_a = to_outline( *before );
	const auto shapes_b = to_outline( *after );
	REQUIRE( shapes_a.size() == shapes_b.size() );

	Morph morph( *before, *after );
	const auto &pairs = morph.pairs();
	CHECK( complete( pairs, shapes_a.size(), shapes_b.size() ) );

	// nothing fades: all six leaves have a partner
	for ( auto p: pairs )
	{
		CHECK( p.a >= 0 );
		CHECK( p.b >= 0 );
	}

	// the leaves that are identical in both pair with their twin, so they only move
	size_t identical = 0;
	for ( auto p: pairs )
	{
		if ( shapes_a[static_cast<size_t>( p.a )].key == shapes_b[static_cast<size_t>( p.b )].key )
		{
			++identical;
		}
	}
	CHECK( identical == 3 ); // x, +, and the fraction bar

	SECTION( "the identical ones stay exactly where they are all the way through" )
	{
		for ( float t: { 0.F, 0.3F, 0.7F, 1.F } )
		{
			morph.set( t );
			const auto drawn = to_outline( morph.visual() );
			REQUIRE( drawn.size() == pairs.size() );
			for ( size_t k = 0; k < pairs.size(); ++k )
			{
				const auto &a = shapes_a[static_cast<size_t>( pairs[k].a )];
				const auto &b = shapes_b[static_cast<size_t>( pairs[k].b )];
				if ( a.key != b.key )
				{
					continue;
				}
				// same contour, same place: bounds don't change
				AABB from;
				AABB now;
				for ( const auto &c: a.contours ) from.add( on_curve_bounds( c ) );
				for ( const auto &c: drawn[k].contours ) now.add( on_curve_bounds( c ) );
				CHECK( now.min.x() == Approx( from.min.x() ).margin( 1e-3 ) );
				CHECK( now.max.y() == Approx( from.max.y() ).margin( 1e-3 ) );
			}
		}
	}
}

TEST_CASE( "Morph by tag follows the parts of a formula, wherever they moved", "[morph]" )
{
	Theme theme = NewComputerModern::create_theme();
	math::Context context( theme );
	using namespace math;

	// The numerator and denominator swap places; matching by tag sends each to its own
	auto before = row( { ord( tag( "left", text( "a" ) ) ), bin( "+" ), ord( tag( "right", text( "b" ) ) ) } ).build( context );
	auto after = row( { ord( tag( "right", text( "b" ) ) ), bin( "+" ), ord( tag( "left", text( "a" ) ) ) } ).build( context );

	const auto shapes_a = to_outline( *before );
	const auto shapes_b = to_outline( *after );

	MorphOptions options;
	options.leaves = LeafMatch::BY_TAG;
	Morph morph( *before, *after, options );

	auto tag_of = []( const OutlineShape &s, const std::string &name )
	{
		return std::find( s.tags.begin(), s.tags.end(), name ) != s.tags.end();
	};
	for ( auto p: morph.pairs() )
	{
		if ( p.a >= 0 && p.b >= 0 )
		{
			const auto &a = shapes_a[static_cast<size_t>( p.a )];
			const auto &b = shapes_b[static_cast<size_t>( p.b )];
			CHECK( tag_of( a, "left" ) == tag_of( b, "left" ) );
			CHECK( tag_of( a, "right" ) == tag_of( b, "right" ) );
		}
	}
}

namespace
{
size_t both( const Pairs &pairs )
{
	return static_cast<size_t>( std::count_if( pairs.begin(), pairs.end(), []( LeafPair p ) { return p.a >= 0 && p.b >= 0; } ) );
}

size_t only_a( const Pairs &pairs )
{
	return static_cast<size_t>( std::count_if( pairs.begin(), pairs.end(), []( LeafPair p ) { return p.a >= 0 && p.b < 0; } ) );
}

size_t only_b( const Pairs &pairs )
{
	return static_cast<size_t>( std::count_if( pairs.begin(), pairs.end(), []( LeafPair p ) { return p.a < 0 && p.b >= 0; } ) );
}
}

TEST_CASE( "is_word_tag recognises the tags of laid-out words", "[morph]" )
{
	CHECK( is_word_tag( word_tag( 0 ) ) );
	CHECK( is_word_tag( word_tag( 42 ) ) );
	CHECK_FALSE( is_word_tag( "word" ) );
	CHECK_FALSE( is_word_tag( "words" ) );
	CHECK_FALSE( is_word_tag( "word3x" ) );
	CHECK_FALSE( is_word_tag( "numerator" ) );
}

TEST_CASE( "Text morphs match whole words", "[morph]" )
{
	Theme theme = NewComputerModern::create_theme();
	const FontStyle style{ Intent::PROSE };
	auto shapes = [&]( const char *s ) { return to_outline( *make_text( theme, s, style ) ); };

	SECTION( "a word that goes away fades, the others are kept" )
	{
		const auto a = shapes( "the big cat" );
		const auto b = shapes( "the cat" );
		REQUIRE( a.size() == 9 );
		REQUIRE( b.size() == 6 );

		const auto pairs = match_leaves( a, b, LeafMatch::BY_WORD );
		CHECK( complete( pairs, 9, 6 ) );
		CHECK( both( pairs ) == 6 );
		CHECK( only_a( pairs ) == 3 ); // "big"
		CHECK( only_b( pairs ) == 0 );

		// and what was kept is the same letters: they only move
		for ( auto p: pairs )
		{
			if ( p.a >= 0 && p.b >= 0 )
			{
				CHECK( a[static_cast<size_t>( p.a )].key == b[static_cast<size_t>( p.b )].key );
			}
		}
	}

	SECTION( "a word that appears fades in" )
	{
		const auto pairs = match_leaves( shapes( "a cat" ), shapes( "a big cat" ), LeafMatch::BY_WORD );
		CHECK( both( pairs ) == 4 ); // a, c, a, t
		CHECK( only_b( pairs ) == 3 );
		CHECK( only_a( pairs ) == 0 );
	}

	SECTION( "different words between identical ones pair in order, letter by letter" )
	{
		const auto a = shapes( "one two three" );
		const auto b = shapes( "uno dos" );
		const auto pairs = match_leaves( a, b, LeafMatch::BY_WORD );
		CHECK( complete( pairs, a.size(), b.size() ) );
		CHECK( both( pairs ) == 6 );      // one-uno and two-dos
		CHECK( only_a( pairs ) == 5 );    // "three" fades
	}

	SECTION( "a replaced word turns into its replacement, the rest stays" )
	{
		const auto a = shapes( "a big cat" );
		const auto b = shapes( "a red cat" );
		const auto pairs = match_leaves( a, b, LeafMatch::BY_WORD );
		size_t identical = 0;
		for ( auto p: pairs )
		{
			if ( p.a >= 0 && p.b >= 0 && a[static_cast<size_t>( p.a )].key == b[static_cast<size_t>( p.b )].key )
			{
				++identical;
			}
		}
		CHECK( both( pairs ) == 7 );  // a, big-red, cat
		CHECK( identical == 4 );      // a, c, a, t
	}

	SECTION( "AUTO does the same for text" )
	{
		const auto a = shapes( "the big cat" );
		const auto b = shapes( "the cat" );
		const auto pairs = match_leaves( a, b, LeafMatch::AUTO );
		CHECK( both( pairs ) == 6 );
		CHECK( only_a( pairs ) == 3 );
	}

	SECTION( "words that are not laid-out text are left to the other matchers" )
	{
		std::vector<OutlineShape> a{ leaf( 1, 0 ) };
		std::vector<OutlineShape> b{ leaf( 1, 0 ) };
		const auto pairs = match_leaves( a, b, LeafMatch::BY_WORD );
		CHECK( has( pairs, 0, -1 ) );
		CHECK( has( pairs, -1, 0 ) );
	}
}

TEST_CASE( "Formulas made of several pieces of text are not mistaken for one word", "[morph]" )
{
	// Every text() in a formula is laid out on its own, so each starts again at "word0". Matching by that
	// name once paired the "(" of (a+b)^2 with a new b and made the old b fade.
	Theme theme = NewComputerModern::create_theme();
	math::Context context( theme );
	using namespace math;

	const Expr a = tag( "a", text( "a" ) );
	const Expr b = tag( "b", text( "b" ) );
	const Expr expanded = row(
		{ ord( superscript( a, text( "2" ) ) ),
		  bin( "+" ),
		  ord( text( "2ab" ) ),
		  bin( "+" ),
		  ord( superscript( b, text( "2" ) ) ) } );
	auto before = superscript( parens( row( { ord( a ), bin( "+" ), ord( b ) } ) ), text( "2" ) ).build( context );
	auto after = expanded.build( context );

	const auto shapes_a = to_outline( *before );
	const auto shapes_b = to_outline( *after );

	auto has_tag = []( const OutlineShape &s, const char *name )
	{
		return std::find( s.tags.begin(), s.tags.end(), name ) != s.tags.end();
	};
	auto partner_of = [&]( const char *name )
	{
		const auto pairs = match_leaves( shapes_a, shapes_b );
		for ( auto p: pairs )
		{
			if ( p.a >= 0 && p.b >= 0 && has_tag( shapes_a[static_cast<size_t>( p.a )], name ) )
			{
				return p.b;
			}
		}
		return -1;
	};

	SECTION( "each piece of text is its own word" )
	{
		int most = -1;
		for ( const auto &s: shapes_a )
		{
			most = std::max( most, s.word );
		}
		CHECK( most >= 3 ); // (, a, +, b, ), 2: several words, not one
	}

	SECTION( "tagged parts follow their tags" )
	{
		const int a_partner = partner_of( "a" );
		const int b_partner = partner_of( "b" );
		REQUIRE( a_partner >= 0 );
		REQUIRE( b_partner >= 0 );
		CHECK( has_tag( shapes_b[static_cast<size_t>( a_partner )], "a" ) );
		CHECK( has_tag( shapes_b[static_cast<size_t>( b_partner )], "b" ) );
	}

	SECTION( "the parentheses have nothing to become and fade" )
	{
		const auto pairs = match_leaves( shapes_a, shapes_b );
		for ( auto p: pairs )
		{
			if ( p.a >= 0 && ( has_tag( shapes_a[static_cast<size_t>( p.a )], "open" ) || has_tag( shapes_a[static_cast<size_t>( p.a )], "close" ) ) )
			{
				CHECK( p.b < 0 );
			}
		}
	}
}
