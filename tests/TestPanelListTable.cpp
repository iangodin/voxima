#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <themes/NewComputerModern.h>
#include <visuals/CreateList.h>
#include <visuals/CreateTable.h>
#include <visuals/CreateText.h>
#include <visuals/Panel.h>
#include <visuals/Path.h>
#include <visuals/Placement.h>
#include <visuals/Rectangle.h>

using Catch::Approx;

namespace
{
std::unique_ptr<Rectangle> box( float w, float h, float x = 0.F, float y = 0.F )
{
	auto r = std::make_unique<Rectangle>();
	r->half_width = w / 2.F;
	r->half_height = h / 2.F;
	r->position = { x, y };
	return r;
}

// The baseline of a laid-out text: where its origin is
float baseline_of( const Visual &text )
{
	return text.position.y();
}
}

TEST_CASE( "A panel fits its content with the padding, behind it", "[panel]" )
{
	auto panel = make_panel( box( 4.F, 2.F, 10.F, 5.F ), { .padding = 0.5F, .radius = 0.3F } );

	// Two children: the background first (behind), then the content
	REQUIRE( panel->size() == 2 );
	CHECK( ( *panel->begin() ).get() == &panel->background() );
	CHECK( ( *( panel->begin() + 1 ) ).get() == &panel->content() );

	const RoundRectangle &bg = panel->background();
	CHECK( bg.half_width == Approx( 2.5F ) );   // 4 / 2 + 0.5
	CHECK( bg.half_height == Approx( 1.5F ) );  // 2 / 2 + 0.5
	CHECK( bg.position.x() == Approx( 10.F ) ); // on the content's middle
	CHECK( bg.position.y() == Approx( 5.F ) );
	CHECK( bg.top_left_radius == Approx( 0.3F ) );
	CHECK( bg.stroke_thickness > 0.F );
}

TEST_CASE( "A panel's corners are never rounder than it is small, and fit() follows the content", "[panel]" )
{
	auto panel = make_panel( box( 0.2F, 0.2F ), { .padding = 0.05F, .radius = 5.F } );
	CHECK( panel->background().top_left_radius == Approx( 0.15F ) ); // half of 0.3

	panel->content().position = { 3.F, 0.F };
	panel->fit();
	CHECK( panel->background().position.x() == Approx( 3.F ) );

	auto empty = make_panel( std::make_unique<Group>() );
	CHECK( empty->background().half_width == 0.F );
}

TEST_CASE( "A list has a row for each item, lined up, evenly spaced", "[list]" )
{
	Theme theme = NewComputerModern::create_theme();
	const float em = theme.get_stack( FontStyle{ Intent::PROSE } ).font( 0 ).scale;

	auto list = make_list( theme, { "first", "second", "gypsy quilt", "fourth" } ); // (descenders in one of them)
	REQUIRE( list->count() == 4 );

	// Each row steps down by the same distance, whatever letters are in it: a line and the spacing
	const float step = baseline_of( list->row( 0 ) ) - baseline_of( list->row( 1 ) );
	CHECK( step == Approx( ( 1.2F + 0.45F ) * em ) );
	CHECK( baseline_of( list->row( 1 ) ) - baseline_of( list->row( 2 ) ) == Approx( step ) );
	CHECK( baseline_of( list->row( 2 ) ) - baseline_of( list->row( 3 ) ) == Approx( step ) );

	// The bullets are lined up, and the text is: a row is a marker and its text
	REQUIRE( list->row( 0 ).size() == 2 );
	const auto marker_left = []( const Group &row ) { return box_of( **row.begin() ).min.x() + row.position.x() * 0.F; };
	const auto text_left = []( const Group &row ) { return box_of( **( row.begin() + 1 ) ).min.x(); };
	for ( size_t i = 1; i < 4; ++i )
	{
		CHECK( marker_left( list->row( i ) ) == Approx( marker_left( list->row( 0 ) ) ) );
		CHECK( text_left( list->row( i ) ) == Approx( text_left( list->row( 0 ) ) ) );
	}
	CHECK( text_left( list->row( 0 ) ) > marker_left( list->row( 0 ) ) );

	// The middle of the list is the origin
	AABB covered;
	for ( const auto &row: *list )
		covered.add( box_of( *row ) );
	CHECK( ( covered.min.x() + covered.max.x() ) / 2.F == Approx( 0.F ).margin( 1e-3 ) );
}

TEST_CASE( "A numbered list lines its numbers up on the right, and an item can be several lines", "[list]" )
{
	Theme theme = NewComputerModern::create_theme();
	const float em = theme.get_stack( FontStyle{ Intent::PROSE } ).font( 0 ).scale;

	std::vector<std::string> items;
	for ( int i = 0; i < 10; ++i )
		items.push_back( "item " + std::to_string( i + 1 ) );
	items[1] = "two\nlines";
	auto list = make_list( theme, items, { .numbered = true } );
	REQUIRE( list->count() == 10 );

	// 9. and 10. end at the same place (on their dots), so they line up as numbers do
	const auto marker_right = [&]( size_t i ) { return box_of( **list->row( i ).begin() ).max.x(); };
	CHECK( marker_right( 8 ) == Approx( marker_right( 9 ) ) );
	CHECK( marker_right( 0 ) == Approx( marker_right( 9 ) ) );

	// The two-line item takes two lines before the next
	CHECK( baseline_of( list->row( 1 ) ) - baseline_of( list->row( 2 ) ) == Approx( ( 2.F * 1.2F + 0.45F ) * em ) );
	CHECK( baseline_of( list->row( 0 ) ) - baseline_of( list->row( 1 ) ) == Approx( ( 1.F * 1.2F + 0.45F ) * em ) );
}

TEST_CASE( "A table's cells share a baseline in a row, and line up in their columns", "[table]" )
{
	Theme theme = NewComputerModern::create_theme();
	const float em = theme.get_stack( FontStyle{ Intent::PROSE } ).font( 0 ).scale;

	auto table = make_table(
		theme,
		{ { "name", "n", "total" }, { "a", "10", "gypsy" }, { "longer name", "5", "x" } },
		{ .alignment = { Align::START, Align::CENTER, Align::END } } );
	REQUIRE( table->rows() == 3 );
	REQUIRE( table->columns() == 3 );

	// A row: one baseline. Rows: a fixed step apart (a line and the spacing), whatever letters are in them
	for ( size_t r = 0; r < 3; ++r )
	{
		CHECK( table->cell( r, 0 ).position.y() == Approx( table->cell( r, 1 ).position.y() ) );
		CHECK( table->cell( r, 0 ).position.y() == Approx( table->cell( r, 2 ).position.y() ) );
	}
	const float step = table->cell( 0, 0 ).position.y() - table->cell( 1, 0 ).position.y();
	CHECK( step == Approx( ( 1.2F + 0.5F ) * em ) );
	CHECK( table->cell( 1, 0 ).position.y() - table->cell( 2, 0 ).position.y() == Approx( step ) );

	// Columns: left, middle, right, as asked
	CHECK( box_of( table->cell( 0, 0 ) ).min.x() == Approx( box_of( table->cell( 2, 0 ) ).min.x() ) );
	CHECK( ( box_of( table->cell( 1, 1 ) ).min.x() + box_of( table->cell( 1, 1 ) ).max.x() ) / 2.F
		   == Approx( ( box_of( table->cell( 2, 1 ) ).min.x() + box_of( table->cell( 2, 1 ) ).max.x() ) / 2.F ) );
	CHECK( box_of( table->cell( 0, 2 ) ).max.x() == Approx( box_of( table->cell( 1, 2 ) ).max.x() ) );
	CHECK( box_of( table->cell( 1, 2 ) ).max.x() == Approx( box_of( table->cell( 2, 2 ) ).max.x() ) );

	// A column is as wide as its widest: the second column starts after "longer name" and a gap
	CHECK( box_of( table->cell( 0, 1 ) ).min.x() > box_of( table->cell( 2, 0 ) ).max.x() );
}

TEST_CASE( "A table's rules are one path, drawn behind the text", "[table]" )
{
	Theme theme = NewComputerModern::create_theme();
	const std::vector<std::vector<std::string>> rows = { { "a", "b" }, { "c", "d" }, { "e", "f" } };

	auto count_lines = []( const Table &t )
	{
		const auto &rules = dynamic_cast<const Group &>( **t.begin() ); // the first child, behind the text
		size_t lines = 0;
		for ( const auto &rule: rules )
		{
			REQUIRE( dynamic_cast<const Path *>( rule.get() ) );
			for ( const auto &command: dynamic_cast<const Path &>( *rule ).commands )
				lines += std::holds_alternative<LineTo>( command ) ? 1 : 0;
		}
		return lines;
	};

	CHECK( count_lines( *make_table( theme, rows ) ) == 1 );                                // under the header
	CHECK( count_lines( *make_table( theme, rows, { .header_rule = false } ) ) == 0 );      // none
	CHECK( count_lines( *make_table( theme, rows, { .grid = true } ) ) == 4 + 3 );          // 3 rows: 4 horizontal; 2 columns: 3 vertical

	// The middle of the table is the origin
	auto table = make_table( theme, rows );
	AABB covered;
	for ( size_t r = 0; r < 3; ++r )
		for ( size_t c = 0; c < 2; ++c )
			covered.add( box_of( table->cell( r, c ) ) );
	CHECK( ( covered.min.x() + covered.max.x() ) / 2.F == Approx( 0.F ).margin( 1e-3 ) );
	CHECK( ( covered.min.y() + covered.max.y() ) / 2.F == Approx( 0.F ).margin( 1e-3 ) );
}
