#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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

#include <visuals/Layout.h>
#include <visuals/WrapText.h>

TEST_CASE( "wrap_lines breaks text at spaces so no line is wider than asked", "[wrap]" )
{
	Theme theme = NewComputerModern::create_theme();
	const std::string text = "the quick brown fox jumps over the lazy dog and keeps running";
	const FontStyle style{ Intent::PROSE };

	const float natural = layout_extent( *make_text( theme, text, style ) ).max.x() - layout_extent( *make_text( theme, text, style ) ).min.x();
	const float limit = natural / 3.F;
	const std::string wrapped = wrap_lines( theme, text, style, limit );

	CHECK( wrapped.find( '\n' ) != std::string::npos );
	size_t start = 0;
	int lines = 0;
	while ( start <= wrapped.size() )
	{
		size_t end = wrapped.find( '\n', start );
		if ( end == std::string::npos )
		{
			end = wrapped.size();
		}
		const AABB box = layout_extent( *make_text( theme, wrapped.substr( start, end - start ), style ) );
		CHECK( box.max.x() - box.min.x() <= limit + 1e-3F );
		++lines;
		start = end + 1;
	}
	CHECK( lines >= 3 );

	// Nothing lost but the spaces where the lines break
	std::string joined = wrapped;
	std::replace( joined.begin(), joined.end(), '\n', ' ' );
	CHECK( joined == text );
}

TEST_CASE( "wrap_lines leaves a text that fits, and keeps a word too long on its own line", "[wrap]" )
{
	Theme theme = NewComputerModern::create_theme();
	const FontStyle style{ Intent::PROSE };
	CHECK( wrap_lines( theme, "short text", style, 100.F ) == "short text" );
	CHECK( wrap_lines( theme, "a extraordinarily b", style, 0.5F ) == "a\nextraordinarily\nb" );
	CHECK( wrap_lines( theme, "one\ntwo", style, 100.F ) == "one\ntwo" );
}

TEST_CASE( "a list wraps its items to a maximum width", "[list][wrap]" )
{
	Theme theme = NewComputerModern::create_theme();
	const std::vector<std::string> items = { "a fairly long first item that will not fit on one line", "short" };
	auto loose = make_list( theme, items );
	const AABB loose_box = box_of( *loose );
	const float width = ( loose_box.max.x() - loose_box.min.x() ) / 2.F;

	auto wrapped = make_list( theme, items, { .max_width = width } );
	const AABB box = box_of( *wrapped );
	CHECK( box.max.x() - box.min.x() <= width + 1e-3F );
	CHECK( box.max.y() - box.min.y() > loose_box.max.y() - loose_box.min.y() ); // taller: more lines
	// The second item starts lower than it did, the first now taking several lines
	CHECK( wrapped->row( 1 ).position.y() < loose->row( 1 ).position.y() );
}

TEST_CASE( "a panel's shadow is behind everything, offset, and fades out past the edge", "[panel]" )
{
	auto plain = make_panel( box( 4.F, 2.F ) );
	CHECK( plain->size() == 2 ); // no shadow: as it always was

	auto panel = make_panel( box( 4.F, 2.F ), { .padding = 0.5F, .shadow = ShadowOptions{ .offset = { 0.1F, -0.2F }, .blur = 0.4F, .layers = 4 } } );
	REQUIRE( panel->size() == 3 );
	const auto &shadow = dynamic_cast<const Group &>( **panel->begin() ); // first: at the back
	REQUIRE( shadow.size() == 4 );

	const RoundRectangle &bg = panel->background();
	float previous = 0.F;
	for ( const auto &child: shadow )
	{
		const auto &layer = dynamic_cast<const RoundRectangle &>( *child );
		CHECK( layer.position.x() == Approx( bg.position.x() + 0.1F ) );
		CHECK( layer.position.y() == Approx( bg.position.y() - 0.2F ) );
		CHECK( layer.half_width > previous ); // each a little bigger than the last
		previous = layer.half_width;
		CHECK( layer.color.a == Approx( 0.45F / 4.F ) ); // so together they are as dark as asked at the middle
	}
	// The biggest reaches half the blur past the panel's edge; the smallest is inside it
	const auto &biggest = dynamic_cast<const RoundRectangle &>( **( shadow.begin() + 3 ) );
	CHECK( biggest.half_width == Approx( bg.half_width + 0.4F * ( 3.5F / 4.F - 0.5F ) ) );
	CHECK( dynamic_cast<const RoundRectangle &>( **shadow.begin() ).half_width < bg.half_width );
}

TEST_CASE( "a table wraps its widest columns to fit a maximum width", "[table][wrap]" )
{
	Theme theme = NewComputerModern::create_theme();
	const std::vector<std::vector<std::string>> rows = {
		{ "Name", "Notes" },
		{ "one", "a long note that goes on and on for quite a while" },
		{ "two", "short" } };
	auto loose = make_table( theme, rows );
	const AABB loose_box = box_of( *loose );
	const float width = ( loose_box.max.x() - loose_box.min.x() ) * 0.6F;

	auto tight = make_table( theme, rows, { .max_width = width } );
	const AABB box = box_of( *tight );
	{
		AABB cells;
		for ( size_t r = 0; r < 3; ++r )
			for ( size_t c = 0; c < 2; ++c )
				cells.add( box_of( tight->cell( r, c ) ).min ), cells.add( box_of( tight->cell( r, c ) ).max );
		// (the cells: the rule under the header reaches half a gap past them each side)
		CHECK( cells.max.x() - cells.min.x() <= width + 1e-3F );
	}
	CHECK( box.max.y() - box.min.y() > loose_box.max.y() - loose_box.min.y() ); // taller: the note took more lines
	// The row after the wrapped one is lower than it was, and the short column was left alone
	CHECK( tight->cell( 2, 0 ).position.y() < loose->cell( 2, 0 ).position.y() );

	// A table that already fits is not changed
	auto same = make_table( theme, rows, { .max_width = 1000.F } );
	CHECK( box_of( *same ).max.x() - box_of( *same ).min.x() == Approx( loose_box.max.x() - loose_box.min.x() ) );
}

TEST_CASE( "a table's cells can be any visuals", "[table]" )
{
	Theme theme = NewComputerModern::create_theme();
	std::vector<std::vector<std::unique_ptr<Visual>>> cells( 2 );
	cells[0].push_back( box( 1.F, 0.5F ) );
	cells[0].push_back( box( 3.F, 0.5F ) );
	cells[1].push_back( box( 2.F, 0.5F ) );
	cells[1].push_back( box( 1.F, 0.5F ) );
	auto table = make_table( theme, std::move( cells ), { .header = false } );

	CHECK( table->rows() == 2 );
	CHECK( table->columns() == 2 );
	// Column 0 is as wide as its widest cell, column 1 begins after it and the gap
	const AABB a = box_of( table->cell( 0, 0 ) );
	const AABB b = box_of( table->cell( 0, 1 ) );
	const AABB c = box_of( table->cell( 1, 0 ) );
	CHECK( b.min.x() > c.max.x() ); // (the second column is past the widest of the first)
	CHECK( a.min.x() == Approx( c.min.x() ) ); // left aligned
}

TEST_CASE( "a table can tint its rows, behind everything", "[table]" )
{
	Theme theme = NewComputerModern::create_theme();
	const std::vector<std::vector<std::string>> rows = { { "h" }, { "a" }, { "b" }, { "c" } };
	auto plain = make_table( theme, rows );
	const auto &plain_rules = dynamic_cast<const Group &>( **plain->begin() );
	CHECK( plain_rules.size() == 1 ); // just the rule under the header

	auto striped = make_table( theme, rows, { .striped = true, .header_fill = Color{ 1.F, 0.F, 0.F, 0.2F } } );
	const auto &rules = dynamic_cast<const Group &>( **striped->begin() );
	// The header's band, the second body row's, then the rule
	REQUIRE( rules.size() == 3 );
	const auto &header_band = dynamic_cast<const Rectangle &>( **rules.begin() );
	const auto &stripe = dynamic_cast<const Rectangle &>( **( rules.begin() + 1 ) );
	CHECK( header_band.color.r == Approx( 1.F ) );
	CHECK( stripe.position.y() < header_band.position.y() );
	// Each band is as tall as the row's step, and the same width
	CHECK( stripe.half_width == Approx( header_band.half_width ) );
	CHECK( stripe.half_height > 0.2F );
}

TEST_CASE( "a row of tall cells is as tall as they are, above and below the baseline", "[table]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto make = [&]( float first_height ) {
		std::vector<std::vector<std::unique_ptr<Visual>>> cells( 2 );
		cells[0].push_back( box( 1.F, first_height ) ); // centered on its origin: reaches half its height each way
		cells[1].push_back( box( 1.F, 0.2F ) );
		return make_table( theme, std::move( cells ), { .header = false } );
	};
	auto small = make( 0.2F );
	auto tall = make( 3.F );
	// The second row is lower by what the first grew below its baseline (1.5 against the 0.1 it had)
	const float small_step = small->cell( 0, 0 ).position.y() - small->cell( 1, 0 ).position.y();
	const float tall_step = tall->cell( 0, 0 ).position.y() - tall->cell( 1, 0 ).position.y();
	CHECK( tall_step > small_step + 1.F );
	// And the table is taller by the same room, above the first row too
	CHECK( box_of( *tall ).max.y() - box_of( *tall ).min.y() > box_of( *small ).max.y() - box_of( *small ).min.y() + 2.F );
}
