#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <core/Theme.h>
#include <plot/BarChart.h>
#include <themes/NewComputerModern.h>
#include <visuals/Placement.h>

using Catch::Approx;

TEST_CASE( "bars stand on the baseline, as tall as their values say", "[barchart]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto chart = make_bar_chart( theme, { .categories = { "a", "b", "c" }, .series = { { "s", { 2, 4, 3 } } }, .width = 6.F, .height = 4.F } );

	REQUIRE( chart->series_count() == 1 );
	REQUIRE( chart->category_count() == 3 );
	CHECK( chart->value_range().min == Approx( 0.0 ) );
	CHECK( chart->value_range().max >= 4.0 );

	const Axes &axes = chart->axes();
	const Rectangle &tall = *chart->bar( 0, 1 );
	const Rectangle &short_bar = *chart->bar( 0, 0 );
	// Each rests on zero, and heights are in proportion to the values
	CHECK( ( tall.position.y() - tall.half_height ) == Approx( axes.at( 0, 0 ).y() ).margin( 1e-3 ) );
	CHECK( ( short_bar.position.y() - short_bar.half_height ) == Approx( axes.at( 0, 0 ).y() ).margin( 1e-3 ) );
	CHECK( tall.half_height == Approx( short_bar.half_height * 2.F ) );

	// One to a category, each in its own slot, left to right
	CHECK( chart->bar( 0, 0 )->position.x() < chart->bar( 0, 1 )->position.x() );
	CHECK( chart->bar( 0, 1 )->position.x() < chart->bar( 0, 2 )->position.x() );
	CHECK( chart->bar( 0, 1 )->position.x() == Approx( axes.at( 1.5, 0 ).x() ).margin( 1e-3 ) );

	CHECK( chart->bar( 0, 5 ) == nullptr );
	CHECK( chart->bar( 3, 0 ) == nullptr );
}

TEST_CASE( "several series are side by side in a category, or stacked", "[barchart]" )
{
	Theme theme = NewComputerModern::create_theme();
	BarChartOptions options;
	options.categories = { "a", "b" };
	options.series = { { "x", { 3, 1 } }, { "y", { 2, 2 } } };
	options.width = 6.F;
	options.height = 4.F;

	auto beside = make_bar_chart( theme, options );
	// Side by side: two bars in the slot, the first to the left, both resting on zero
	CHECK( beside->bar( 0, 0 )->position.x() < beside->bar( 1, 0 )->position.x() );
	CHECK( beside->bar( 0, 0 )->position.y() - beside->bar( 0, 0 )->half_height == Approx( beside->bar( 1, 0 )->position.y() - beside->bar( 1, 0 )->half_height ).margin( 1e-3 ) );
	CHECK( beside->value_range().max >= 3.0 );

	options.stacked = true;
	auto stacked = make_bar_chart( theme, options );
	// Stacked: one over the other, the same place across, and the range reaches their sum
	CHECK( stacked->bar( 0, 0 )->position.x() == Approx( stacked->bar( 1, 0 )->position.x() ) );
	CHECK( stacked->bar( 1, 0 )->position.y() > stacked->bar( 0, 0 )->position.y() );
	CHECK( stacked->bar( 0, 0 )->position.y() + stacked->bar( 0, 0 )->half_height == Approx( stacked->bar( 1, 0 )->position.y() - stacked->bar( 1, 0 )->half_height ).margin( 1e-3 ) );
	CHECK( stacked->value_range().max >= 5.0 );
}

TEST_CASE( "horizontal bars run across, the first category at the top", "[barchart]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto chart = make_bar_chart( theme, { .categories = { "first", "second" }, .series = { { "s", { 4, 2 } } }, .horizontal = true } );
	const Rectangle &first = *chart->bar( 0, 0 );
	const Rectangle &second = *chart->bar( 0, 1 );
	CHECK( first.position.y() > second.position.y() );
	CHECK( first.half_width == Approx( second.half_width * 2.F ) );
	// They start at the left, at zero
	CHECK( first.position.x() - first.half_width == Approx( chart->axes().at( 0, 0 ).x() ).margin( 1e-3 ) );
}

TEST_CASE( "negative values hang below a zero line, and the range takes them in", "[barchart]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto chart = make_bar_chart( theme, { .categories = { "a", "b" }, .series = { { "s", { 3, -2 } } } } );
	CHECK( chart->value_range().min <= -2.0 );
	CHECK( chart->value_range().max >= 3.0 );
	const Rectangle &down = *chart->bar( 0, 1 );
	CHECK( down.position.y() + down.half_height == Approx( chart->axes().at( 0, 0 ).y() ).margin( 1e-3 ) ); // its top is at zero
}

TEST_CASE( "a series shorter than the categories leaves bars out, and colors follow the series or the category", "[barchart]" )
{
	Theme theme = NewComputerModern::create_theme();
	auto chart = make_bar_chart( theme, { .categories = { "a", "b", "c" }, .series = { { "s", { 1, 2 } }, { "t", { 2, 1, 3 } } } } );
	CHECK( chart->bar( 0, 2 ) == nullptr );
	CHECK( chart->bar( 1, 2 ) != nullptr );
	const Color first = chart->series_color( 0 );
	const Color second = chart->series_color( 1 );
	CHECK( ( first.r != second.r || first.g != second.g || first.b != second.b ) );
	CHECK( chart->bar( 0, 0 )->color.r == first.r );

	auto by_category = make_bar_chart( theme, { .categories = { "a", "b" }, .series = { { "s", { 1, 2 } } }, .color_by_category = true } );
	const Color c0 = by_category->bar( 0, 0 )->color;
	const Color c1 = by_category->bar( 0, 1 )->color;
	CHECK( ( c0.r != c1.r || c0.g != c1.g || c0.b != c1.b ) );
}
