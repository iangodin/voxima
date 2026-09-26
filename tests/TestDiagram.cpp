#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <visuals/Diagram.h>
#include <visuals/Placement.h>
#include <visuals/Rectangle.h>

using Catch::Approx;

namespace
{
std::unique_ptr<Rectangle> box( float w, float h )
{
	auto r = std::make_unique<Rectangle>();
	r->half_width = w / 2.F;
	r->half_height = h / 2.F;
	return r;
}

// a -> b, a -> c, b -> d, c -> d
void diamond( Diagram &d )
{
	d.add_node( "a", box( 2.F, 1.F ) );
	d.add_node( "b", box( 2.F, 1.F ) );
	d.add_node( "c", box( 4.F, 1.F ) );
	d.add_node( "d", box( 2.F, 1.F ) );
	d.add_edge( "a", "b" );
	d.add_edge( "a", "c" );
	d.add_edge( "b", "d" );
	d.add_edge( "c", "d" );
}
}

TEST_CASE( "a diagram finds its nodes by id and refuses a repeat or an unknown id", "[diagram]" )
{
	Diagram d;
	diamond( d );
	CHECK( d.node_count() == 4 );
	CHECK( d.edge_count() == 4 );
	CHECK( d.has_node( "c" ) );
	CHECK( !d.has_node( "z" ) );
	CHECK_THROWS( d.add_node( "a", box( 1.F, 1.F ) ) );
	CHECK_THROWS( d.add_edge( "a", "z" ) );
	CHECK_THROWS( d.node( "z" ) );
}

TEST_CASE( "layout_layers puts each node a layer after the ones that point at it", "[diagram]" )
{
	Diagram d;
	diamond( d );
	d.layout_layers();

	const float a = point_at( d.node( "a" ), Anchor::CENTER ).y();
	const float b = point_at( d.node( "b" ), Anchor::CENTER ).y();
	const float c = point_at( d.node( "c" ), Anchor::CENTER ).y();
	const float e = point_at( d.node( "d" ), Anchor::CENTER ).y();
	CHECK( a > b );
	CHECK( b == Approx( c ) ); // the same layer
	CHECK( b > e );

	// Side by side in their layer, in the order added, with the gap between (and centered)
	const AABB bb = box_of( d.node( "b" ) );
	const AABB cb = box_of( d.node( "c" ) );
	CHECK( bb.max.x() < cb.min.x() );
	CHECK( cb.min.x() - bb.max.x() == Approx( 0.6F ) );
	CHECK( ( bb.min.x() + cb.max.x() ) / 2.F == Approx( 0.F ).margin( 1e-3 ) );
}

TEST_CASE( "layout_layers can run to the right, and the arrows follow the nodes", "[diagram]" )
{
	Diagram d;
	diamond( d );
	d.layout_layers( { .direction = Direction::RIGHT } );
	CHECK( point_at( d.node( "a" ), Anchor::CENTER ).x() < point_at( d.node( "b" ), Anchor::CENTER ).x() );

	// The first arrow ends at b's edge
	const AABB b = box_of( d.node( "b" ) );
	CHECK( d.edge( 0 ).end_tip().end.x() <= b.min.x() );
	CHECK( d.edge( 0 ).end_tip().end.x() > b.min.x() - 0.5F );
}

TEST_CASE( "layout_layers copes with arrows that loop back", "[diagram]" )
{
	Diagram d;
	d.add_node( "a", box( 1.F, 1.F ) );
	d.add_node( "b", box( 1.F, 1.F ) );
	d.add_edge( "a", "b" );
	d.add_edge( "b", "a" );
	d.layout_layers();
	CHECK( d.node_count() == 2 );
}
