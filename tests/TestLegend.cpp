#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <plot/Legend.h>
#include <themes/NewComputerModern.h>
#include <visuals/Placement.h>

#include <cmath>

using Catch::Approx;

namespace
{
std::vector<LegendEntry> three()
{
	return { { "first", { 1.F, 0.F, 0.F, 1.F }, Swatch::SQUARE },
			 { "a much longer second", { 0.F, 1.F, 0.F, 1.F }, Swatch::LINE },
			 { "third", { 0.F, 0.F, 1.F, 1.F }, Swatch::LINE_MARKER, MarkerShape::TRIANGLE } };
}
}

TEST_CASE( "a legend has a swatch and a name for each entry, the names in a column", "[legend]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto legend = make_legend( theme, three() );
	REQUIRE( legend->entry_count() == 3 );
	REQUIRE( legend->panel() != nullptr );

	// The names start in one column, whatever the width of the swatch before them
	const float left0 = legend->label_box( 0 ).min.x();
	const float left1 = legend->label_box( 1 ).min.x();
	CHECK( left0 == Approx( left1 ).margin( 0.05 ) );

	// One under another, evenly
	const float y0 = legend->swatch_center( 0 ).y();
	const float y1 = legend->swatch_center( 1 ).y();
	const float y2 = legend->swatch_center( 2 ).y();
	CHECK( y0 > y1 );
	CHECK( y0 - y1 == Approx( y1 - y2 ).margin( 1e-3 ) );

	// The swatches are centered in their column: all on one vertical line
	CHECK( legend->swatch_center( 0 ).x() == Approx( legend->swatch_center( 1 ).x() ).margin( 1e-3 ) );
	CHECK( legend->swatch_center( 2 ).x() == Approx( legend->swatch_center( 1 ).x() ).margin( 1e-3 ) );
}

TEST_CASE( "a legend can lie side by side, or go without its panel", "[legend]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto across = make_legend( theme, three(), { .horizontal = true, .panel = false } );
	CHECK( across->panel() == nullptr );
	CHECK( across->swatch_center( 0 ).x() < across->swatch_center( 1 ).x() );
	CHECK( across->swatch_center( 1 ).x() < across->swatch_center( 2 ).x() );
	CHECK( across->swatch_center( 0 ).y() == Approx( across->swatch_center( 2 ).y() ).margin( 1e-3 ) );
}

TEST_CASE( "a legend is even round its entries, whatever letters they hold", "[legend]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto legend = make_legend( theme, { { "gy", { 1.F, 0.F, 0.F, 1.F } }, { "ab", { 0.F, 1.F, 0.F, 1.F } } } );
	const AABB panel = box_of( *legend->panel() );
	const float above = panel.max.y() - legend->swatch_center( 0 ).y();
	const float below = legend->swatch_center( 1 ).y() - panel.min.y();
	// (about: the rows are a line tall, centered on the swatches)
	CHECK( above == Approx( below ).margin( 0.02 ) );
}

TEST_CASE( "entries come from what is plotted, in its colors", "[legend]" )
{
	Theme theme = NewComputerModern::create_theme();
	AxesOptions options;
	options.numbers = false;
	auto axes = make_axes( theme, options );

	auto points = scatter( *axes, { { 0.0, 0.0 } }, { .marker = MarkerShape::SQUARE, .name = "dots" } );
	const LegendEntry dots = legend_entry( *points );
	CHECK( dots.name == "dots" );
	CHECK( dots.swatch == Swatch::MARKER );
	CHECK( dots.marker == MarkerShape::SQUARE );
	CHECK( dots.color.r == points->series_color().r );

	auto joined = scatter( *axes, { { 0.0, 0.0 }, { 1.0, 1.0 } }, { .line = JoinStyle::STRAIGHT } );
	CHECK( legend_entry( *joined ).swatch == Swatch::LINE_MARKER );

	auto curve = plot( *axes, []( double x ) { return x; }, { .name = "line" } );
	const LegendEntry line = legend_entry( *curve );
	CHECK( line.name == "line" );
	CHECK( line.swatch == Swatch::LINE );
	CHECK( line.color.g == curve->line_color().g );

	auto chart = make_bar_chart( theme, { .categories = { "a", "b" }, .series = { { "x", { 1, 2 } }, { "y", { 2, 1 } } } } );
	const auto entries = legend_entries( *chart );
	REQUIRE( entries.size() == 2 );
	CHECK( entries[1].name == "y" );
	CHECK( entries[1].color.r == chart->series_color( 1 ).r );

	// One series with a color to each category: the categories are the entries
	auto colored = make_bar_chart( theme, { .categories = { "a", "b", "c" }, .series = { { "x", { 1, 2, 3 } } }, .color_by_category = true } );
	const auto by_category = legend_entries( *colored );
	REQUIRE( by_category.size() == 3 );
	CHECK( by_category[2].name == "c" );
}

TEST_CASE( "add_legend puts a legend in a corner of the plot, inset", "[legend]" )
{
	Theme theme = NewComputerModern::create_theme();
	AxesOptions options;
	options.width = 8.F;
	options.height = 5.F;
	auto axes = make_axes( theme, options );

	struct Case { Anchor corner; float x; float y; };
	for ( const Case &c: { Case{ Anchor::TOP_LEFT, -1.F, 1.F }, Case{ Anchor::TOP_RIGHT, 1.F, 1.F }, Case{ Anchor::BOTTOM_LEFT, -1.F, -1.F }, Case{ Anchor::BOTTOM_RIGHT, 1.F, -1.F } } )
	{
		Legend &legend = add_legend( *axes, make_legend( theme, three() ), c.corner, 0.3F );
		const AABB box = box_of( legend );
		if ( c.x > 0.F )
			CHECK( box.max.x() == Approx( axes->right() - 0.3F ).margin( 1e-3 ) );
		else
			CHECK( box.min.x() == Approx( axes->left() + 0.3F ).margin( 1e-3 ) );
		if ( c.y > 0.F )
			CHECK( box.max.y() == Approx( axes->top() - 0.3F ).margin( 1e-3 ) );
		else
			CHECK( box.min.y() == Approx( axes->bottom() + 0.3F ).margin( 1e-3 ) );
	}
}
