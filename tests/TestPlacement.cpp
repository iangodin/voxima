#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <visuals/Circle.h>
#include <visuals/Placement.h>
#include <visuals/Rectangle.h>

using Catch::Approx;

namespace
{
// A rectangle w wide and h tall, its center at (x, y)
Rectangle &add_box( Group &group, float w, float h, float x = 0.F, float y = 0.F )
{
	auto box = std::make_unique<Rectangle>();
	box->half_width = w / 2.F;
	box->half_height = h / 2.F;
	box->position = { x, y };
	return group.add( std::move( box ) );
}
}

TEST_CASE( "point_at gives the edges, corners and center of a visual's box", "[placement]" )
{
	Group group;
	auto &box = add_box( group, 4.F, 2.F, 10.F, 5.F ); // x from 8 to 12, y from 4 to 6

	CHECK( point_at( box, Anchor::CENTER ).x() == Approx( 10.F ) );
	CHECK( point_at( box, Anchor::CENTER ).y() == Approx( 5.F ) );
	CHECK( point_at( box, Anchor::LEFT ).x() == Approx( 8.F ) );
	CHECK( point_at( box, Anchor::LEFT ).y() == Approx( 5.F ) );
	CHECK( point_at( box, Anchor::RIGHT ).x() == Approx( 12.F ) );
	CHECK( point_at( box, Anchor::TOP ).y() == Approx( 6.F ) );
	CHECK( point_at( box, Anchor::BOTTOM ).y() == Approx( 4.F ) );
	CHECK( point_at( box, Anchor::TOP_LEFT ).x() == Approx( 8.F ) );
	CHECK( point_at( box, Anchor::TOP_LEFT ).y() == Approx( 6.F ) );
	CHECK( point_at( box, Anchor::BOTTOM_RIGHT ).x() == Approx( 12.F ) );
	CHECK( point_at( box, Anchor::BOTTOM_RIGHT ).y() == Approx( 4.F ) );
}

TEST_CASE( "move_to puts a point of a visual where asked", "[placement]" )
{
	Group group;
	auto &box = add_box( group, 4.F, 2.F ); // -2 .. 2, -1 .. 1

	move_to( box, { 0.F, 3.5F }, Anchor::TOP ); // its top edge at y = 3.5
	CHECK( point_at( box, Anchor::TOP ).y() == Approx( 3.5F ) );
	CHECK( point_at( box, Anchor::TOP ).x() == Approx( 0.F ) ); // the middle of the top edge is at x = 0, as it was

	move_to( box, { -6.F, -2.F }, Anchor::BOTTOM_LEFT );
	CHECK( box_of( box ).min.x() == Approx( -6.F ) );
	CHECK( box_of( box ).min.y() == Approx( -2.F ) );
	CHECK( box.position.x() == Approx( -4.F ) ); // its center is 2 across from its left
}

TEST_CASE( "next_to puts a visual beside another with a gap, lined up as asked", "[placement]" )
{
	Group group;
	auto &fixed = add_box( group, 4.F, 2.F, 0.F, 0.F );  // -2 .. 2, -1 .. 1
	auto &small = add_box( group, 1.F, 1.F, 20.F, 20.F );

	next_to( small, fixed, Anchor::RIGHT, 0.5F ); // centered on the row of it
	CHECK( box_of( small ).min.x() == Approx( 2.5F ) );
	CHECK( point_at( small, Anchor::CENTER ).y() == Approx( 0.F ) );

	next_to( small, fixed, Anchor::LEFT, 0.5F, Align::START ); // the tops level
	CHECK( box_of( small ).max.x() == Approx( -2.5F ) );
	CHECK( box_of( small ).max.y() == Approx( 1.F ) );

	next_to( small, fixed, Anchor::BOTTOM, 0.25F, Align::END ); // under it, the right edges level
	CHECK( box_of( small ).max.y() == Approx( -1.25F ) );
	CHECK( box_of( small ).max.x() == Approx( 2.F ) );

	next_to( small, fixed, Anchor::TOP, 0.25F ); // over it, centered
	CHECK( box_of( small ).min.y() == Approx( 1.25F ) );
	CHECK( point_at( small, Anchor::CENTER ).x() == Approx( 0.F ) );
}

TEST_CASE( "arrange_row puts children side by side with a gap, centered", "[placement]" )
{
	Group group;
	auto &a = add_box( group, 2.F, 1.F, 50.F, 9.F ); // wherever they were
	auto &b = add_box( group, 4.F, 3.F, -7.F, 1.F );
	auto &c = add_box( group, 1.F, 2.F, 0.F, 0.F );

	arrange_row( group, { .gap = 0.5F } );

	// In their order, 0.5 apart
	CHECK( box_of( b ).min.x() - box_of( a ).max.x() == Approx( 0.5F ) );
	CHECK( box_of( c ).min.x() - box_of( b ).max.x() == Approx( 0.5F ) );

	// Centered on the origin: 2 + 4 + 1 and two gaps is 8 wide
	CHECK( box_of( a ).min.x() == Approx( -4.F ) );
	CHECK( box_of( c ).max.x() == Approx( 4.F ) );

	// And centered on each other up and down
	CHECK( point_at( a, Anchor::CENTER ).y() == Approx( point_at( b, Anchor::CENTER ).y() ) );
	CHECK( point_at( c, Anchor::CENTER ).y() == Approx( point_at( b, Anchor::CENTER ).y() ) );
	CHECK( point_at( b, Anchor::CENTER ).y() == Approx( 0.F ) );
}

TEST_CASE( "arrange_row can line up tops or bottoms, or leave the block where it began", "[placement]" )
{
	Group group;
	auto &a = add_box( group, 2.F, 1.F );
	auto &b = add_box( group, 2.F, 3.F );

	arrange_row( group, { .align = Align::START } );
	CHECK( box_of( a ).max.y() == Approx( box_of( b ).max.y() ) );

	arrange_row( group, { .align = Align::END } );
	CHECK( box_of( a ).min.y() == Approx( box_of( b ).min.y() ) );

	// Not centered: the first child stays where it was
	Group other;
	auto &first = add_box( other, 2.F, 1.F, 7.F, 7.F );
	add_box( other, 2.F, 1.F );
	arrange_row( other, { .center = false } );
	CHECK( first.position.x() == Approx( 7.F ) );
	CHECK( first.position.y() == Approx( 7.F ) );
}

TEST_CASE( "Align::BASELINE keeps origins level", "[placement]" )
{
	Group group;
	auto &tall = add_box( group, 2.F, 4.F, 0.F, 3.F );
	auto &short_one = add_box( group, 2.F, 1.F, 5.F, 9.F );
	arrange_row( group, { .align = Align::BASELINE, .center = false } );
	CHECK( short_one.position.y() == Approx( tall.position.y() ) );
}

TEST_CASE( "arrange_column stacks children from the top, aligned left, centered or right", "[placement]" )
{
	Group group;
	auto &a = add_box( group, 2.F, 1.F );
	auto &b = add_box( group, 6.F, 2.F );

	arrange_column( group, { .gap = 0.4F, .align = Align::START } );
	CHECK( box_of( a ).min.x() == Approx( box_of( b ).min.x() ) );
	CHECK( box_of( a ).min.y() - box_of( b ).max.y() == Approx( 0.4F ) );
	CHECK( box_of( a ).max.y() > box_of( b ).max.y() );

	// Centered on the origin: 1 + 0.4 + 2 tall
	CHECK( box_of( a ).max.y() == Approx( 1.7F ) );
	CHECK( box_of( b ).min.y() == Approx( -1.7F ) );

	arrange_column( group, { .align = Align::END } );
	CHECK( box_of( a ).max.x() == Approx( box_of( b ).max.x() ) );
	arrange_column( group, { .align = Align::CENTER } );
	CHECK( point_at( a, Anchor::CENTER ).x() == Approx( point_at( b, Anchor::CENTER ).x() ) );
}

TEST_CASE( "arrange_grid lays children in rows, a column as wide as its widest", "[placement]" )
{
	Group group;
	auto &a = add_box( group, 2.F, 1.F );
	auto &b = add_box( group, 4.F, 1.F );
	auto &c = add_box( group, 3.F, 2.F );
	auto &d = add_box( group, 1.F, 1.F );

	arrange_grid( group, 2, { .column_gap = 1.F, .row_gap = 0.5F, .horizontal = Align::START, .vertical = Align::START } );

	// Two columns (3 and 4 wide, 1 apart) and two rows (1 and 2 tall, 0.5 apart)
	CHECK( box_of( a ).min.x() == Approx( box_of( c ).min.x() ) );        // the same column: left edges
	CHECK( box_of( b ).min.x() == Approx( box_of( d ).min.x() ) );
	CHECK( box_of( b ).min.x() - box_of( a ).min.x() == Approx( 3.F + 1.F ) ); // a column is as wide as its widest (c: 3)
	CHECK( box_of( a ).max.y() == Approx( box_of( b ).max.y() ) );        // the same row, START: tops
	CHECK( box_of( a ).min.y() - box_of( c ).max.y() == Approx( 0.5F ) ); // the first row is as tall as a (1), and c starts a gap below it

	// Centered as a block: 3 + 1 + 4 wide, 1 + 0.5 + 2 tall
	CHECK( box_of( a ).min.x() == Approx( -4.F ) );
	CHECK( box_of( b ).max.x() == Approx( 4.F ) );
	CHECK( box_of( a ).max.y() == Approx( 1.75F ) );
	CHECK( box_of( c ).min.y() == Approx( -1.75F ) );

	// Within a cell: centered
	arrange_grid( group, 2, { .horizontal = Align::CENTER, .vertical = Align::CENTER } );
	CHECK( point_at( a, Anchor::CENTER ).x() == Approx( point_at( c, Anchor::CENTER ).x() ) );
	CHECK( point_at( a, Anchor::CENTER ).y() == Approx( point_at( b, Anchor::CENTER ).y() ) );
}

TEST_CASE( "Arranging skips children with nothing to draw", "[placement]" )
{
	Group group;
	auto &a = add_box( group, 2.F, 1.F );
	group.add( std::make_unique<Group>() ); // empty: no box, so no room
	auto &b = add_box( group, 2.F, 1.F );

	arrange_row( group, { .gap = 1.F } );
	CHECK( box_of( b ).min.x() - box_of( a ).max.x() == Approx( 1.F ) );

	Group none;
	arrange_row( none );        // and an empty group is fine
	arrange_grid( none, 3 );
	arrange_grid( group, 0 );
}
