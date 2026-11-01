#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "RenderTestUtil.h"

#include <core/Animate.h>
#include <core/Stage.h>
#include <plot/Axes.h>
#include <plot/Graph.h>
#include <visuals/Circle.h>
#include <visuals/Connector.h>
#include <visuals/Ellipse.h>
#include <visuals/Line.h>
#include <visuals/Morph.h>
#include <visuals/Path.h>
#include <visuals/Rectangle.h>
#include <visuals/RoundRectangle.h>

#include <chrono>
#include <limits>
#include <memory>
#include <string>
#include <vector>

using namespace render_test;
using namespace std::chrono_literals;

namespace
{
bool lengths_are( const StrokeDash &dash, const std::vector<float> &lengths )
{
	if ( dash.pattern.size() != lengths.size() )
	{
		return false;
	}
	for ( size_t i = 0; i < lengths.size(); ++i )
	{
		if ( dash.pattern[i] != Catch::Approx( lengths[i] ) )
		{
			return false;
		}
	}
	return true;
}

bool contains( const std::string &text, const std::string &needle )
{
	return text.find( needle ) != std::string::npos;
}

Line line_from( Point start, Point end, const StrokeDash &dash )
{
	Line line;
	line.start = start;
	line.end = end;
	line.stroke_thickness = 0.1F;
	line.stroke_dash = dash;
	return line;
}

template<typename Shape>
void outlined( Shape &shape, const StrokeDash &dash )
{
	shape.color = { 1.F, 1.F, 1.F, 0.F };
	shape.stroke_thickness = 0.1F;
	shape.stroke_dash = dash;
}
}

TEST_CASE( "Without dashes, circles and ellipses are written as they were", "[dash]" )
{
	Circle circle( 1.F );
	circle.stroke_thickness = 0.1F;
	CHECK( contains( svg_of( circle ), "<circle" ) );
	Ellipse ellipse;
	ellipse.stroke_thickness = 0.1F;
	CHECK( contains( svg_of( ellipse ), "<ellipse" ) );
}

TEST_CASE( "A dashed stroke is drawn with gaps", "[dash]" )
{
	const Line line = line_from( { -4.F, 0.F }, { 4.F, 0.F }, StrokeDash::dashed( 1.F, 1.F ) );
	const auto picture = render( line );
	CHECK( pixel_at( picture, -3.5F, 0.F ).r == 255 ); // the first dash
	CHECK( pixel_at( picture, -2.5F, 0.F ).r == 0 );   // the first gap
	CHECK( pixel_at( picture, -1.5F, 0.F ).r == 255 );
}

TEST_CASE( "A dash's offset moves the dashes along the line", "[dash]" )
{
	StrokeDash dash = StrokeDash::dashed( 1.F, 1.F );
	dash.offset = 1.F;
	const auto picture = render( line_from( { -4.F, 0.F }, { 4.F, 0.F }, dash ) );
	CHECK( pixel_at( picture, -3.5F, 0.F ).r == 0 );
	CHECK( pixel_at( picture, -2.5F, 0.F ).r == 255 );
}

TEST_CASE( "Dots are round, and as wide as the stroke", "[dash]" )
{
	Line line = line_from( { -4.F, 0.F }, { 4.F, 0.F }, StrokeDash::dotted( 1.F ) );
	line.stroke_thickness = 0.5F;
	const auto picture = render( line );
	CHECK( pixel_at( picture, -3.F, 0.F ).r == 255 );  // on a dot
	CHECK( pixel_at( picture, -2.5F, 0.F ).r == 0 );   // between dots
	CHECK( pixel_at( picture, -3.F, 0.2F ).r == 255 ); // the same dot, up its middle
	CHECK( pixel_at( picture, -2.8F, 0.2F ).r == 0 );  // outside it, though not outside a square
}

TEST_CASE( "No dashes means what was written before", "[dash]" )
{
	const Line line = line_from( { 0.F, 0.F }, { 1.F, 0.F }, {} );
	CHECK_FALSE( contains( svg_of( line ), "dash" ) );
	CHECK_FALSE( contains( svg_of( line ), "linecap" ) );
}

TEST_CASE( "Dashes with no gaps look like a whole line", "[dash]" )
{
	const auto picture = render( line_from( { -4.F, 0.F }, { 4.F, 0.F }, StrokeDash{ { 0.37F, 0.F } } ) );
	for ( float x = -3.9F; x < 3.9F; x += 0.01F )
	{
		REQUIRE( pixel_at( picture, x, 0.F ).r > 200 );
	}
}

TEST_CASE( "A dash pattern needs a stroke to be written", "[dash]" )
{
	Line line = line_from( { 0.F, 0.F }, { 1.F, 0.F }, StrokeDash::dashed( 1.F, 1.F ) );
	line.stroke_thickness = 0.F;
	CHECK_FALSE( contains( svg_of( line ), "dash" ) );
}

TEST_CASE( "A dash pattern that cannot be drawn is a whole line", "[dash]" )
{
	const float pixel = 16.F / 1600.F;
	const float flatness = pixel / 25.F;
	const auto written = [&]( const StrokeDash &dash )
	{
		return contains( svg_of( line_from( { -8.F, 0.F }, { 8.F, 0.F }, dash ), flatness ), "dasharray" );
	};

	CHECK( written( StrokeDash::dashed( 0.1F, 0.1F ) ) );
	SECTION( "too small to see, which the rasteriser would never finish" )
	{
		CHECK_FALSE( written( StrokeDash::dashed( 1e-9F, 1e-9F ) ) );
		CHECK_FALSE( written( StrokeDash::dashed( pixel * 0.2F, pixel * 0.2F ) ) );
		CHECK( written( StrokeDash::dashed( pixel, pixel ) ) );
		// Without knowing the size of a pixel, too
		CHECK_FALSE( contains( svg_of( line_from( { -8.F, 0.F }, { 8.F, 0.F }, StrokeDash::dashed( 1e-9F, 1e-9F ) ) ), "dasharray" ) );
	}
	SECTION( "bad lengths" )
	{
		CHECK_FALSE( written( StrokeDash{ { 0.F, 0.F } } ) );
		CHECK_FALSE( written( StrokeDash{ { -1.F, 1.F } } ) );
		CHECK_FALSE( written( StrokeDash{ { std::numeric_limits<float>::quiet_NaN(), 1.F } } ) );
		CHECK_FALSE( written( StrokeDash{ { std::numeric_limits<float>::infinity(), 1.F } } ) );
	}
	SECTION( "and that is what is seen, without hanging" )
	{
		const auto picture = render( line_from( { -8.F, 0.F }, { 8.F, 0.F }, StrokeDash::dashed( 1e-9F, 1e-9F ) ), flatness );
		CHECK( pixel_at( picture, 0.F, 0.F ).r == 255 );
	}
}

TEST_CASE( "How small is too small takes in the scale of what it is in", "[dash]" )
{
	const float flatness = 0.01F / 25.F;
	Group group;
	group.scale_x = group.scale_y = 1000.F;
	auto &inner = group.add( std::make_unique<Group>() );
	auto &line = inner.add( std::make_unique<Line>() );
	line.stroke_thickness = 0.001F;
	line.end = { 0.01F, 0.F };
	line.stroke_dash = StrokeDash::dashed( 0.001F, 0.001F ); // 2 units on the stage
	CHECK( contains( svg_of( group, flatness ), "dasharray" ) );

	group.scale_x = group.scale_y = 0.1F; // 0.0002 units
	CHECK_FALSE( contains( svg_of( group, flatness ), "dasharray" ) );

	// and it is back to what it was after the group
	group.scale_x = group.scale_y = 0.1F;
	Group both;
	both.add( std::make_unique<Group>() );
	auto &after = both.add( std::make_unique<Line>() );
	after.stroke_thickness = 0.1F;
	after.stroke_dash = StrokeDash::dashed( 1.F, 1.F );
	CHECK( contains( svg_of( both, flatness ), "dasharray" ) );
}

TEST_CASE( "Every shape's outline can be dashed, starting where its outline does", "[dash]" )
{
	const StrokeDash dash = StrokeDash::dashed( 0.3F, 0.2F );
	const auto same_as_its_outline = [&]( Visual &shape )
	{
		// At its start, a morph draws the shape's outline as to_outline gives it. If the dashes began elsewhere they would
		// jump when a morph or a draw_on took over from the shape, or the shape from it.
		Circle other( 0.5F );
		Morph morph( shape, other );
		morph.set( 0.F );
		return pixels_apart( render( shape ), render( morph.visual() ) );
	};

	Circle circle( 2.F );
	outlined( circle, dash );
	CHECK( same_as_its_outline( circle ) < 40 );

	Ellipse ellipse;
	ellipse.half_width = 3.F;
	ellipse.half_height = 1.5F;
	outlined( ellipse, dash );
	CHECK( same_as_its_outline( ellipse ) < 40 );

	Rectangle rectangle( 3.F, 1.5F );
	outlined( rectangle, dash );
	CHECK( same_as_its_outline( rectangle ) < 40 );

	RoundRectangle round;
	round.half_width = 3.F;
	round.half_height = 1.5F;
	round.top_left_radius = round.top_right_radius = round.bottom_left_radius = round.bottom_right_radius = 0.5F;
	outlined( round, dash );
	CHECK( same_as_its_outline( round ) < 100 );

	Path path;
	path.commands = { MoveTo{ { -3.F, 0.F } }, CubicTo{ { 3.F, 0.F }, { -1.F, 3.F }, { 1.F, -3.F } } };
	outlined( path, dash );
	CHECK( same_as_its_outline( path ) < 40 );
}

TEST_CASE( "A morph blends dashes", "[dash][morph]" )
{
	const auto dash_of = []( Morph &morph ) -> const StrokeDash &
	{
		return ( *morph.visual().begin() )->stroke_dash;
	};

	SECTION( "between two patterns, with exactly each at its end" )
	{
		StrokeDash from = StrokeDash::dashed( 0.2F, 0.4F );
		from.offset = 1.F;
		const StrokeDash to = StrokeDash::dashed( 0.6F, 0.8F );
		const Line a = line_from( { 0.F, 0.F }, { 2.F, 0.F }, from );
		const Line b = line_from( { 0.F, 1.F }, { 2.F, 1.F }, to );
		Morph morph( a, b );
		CHECK( dash_of( morph ) == from );
		morph.set( 0.5F );
		CHECK( lengths_are( dash_of( morph ), { 0.4F, 0.6F } ) );
		CHECK( dash_of( morph ).offset == 0.5F );
		morph.set( 1.F );
		CHECK( dash_of( morph ) == to );
	}
	SECTION( "patterns of different lengths are repeated to fit" )
	{
		const Line a = line_from( { 0.F, 0.F }, { 2.F, 0.F }, StrokeDash{ { 0.2F } } );
		const Line b = line_from( { 0.F, 1.F }, { 2.F, 1.F }, StrokeDash{ { 0.2F, 0.2F, 0.6F, 0.6F } } );
		Morph morph( a, b );
		morph.set( 0.5F );
		CHECK( lengths_are( dash_of( morph ), { 0.2F, 0.2F, 0.4F, 0.4F } ) );
	}
	SECTION( "from a whole line, the gaps open" )
	{
		const Line a = line_from( { 0.F, 0.F }, { 2.F, 0.F }, {} );
		const Line b = line_from( { 0.F, 1.F }, { 2.F, 1.F }, StrokeDash::dashed( 0.2F, 0.4F ) );
		Morph morph( a, b );
		CHECK( dash_of( morph ).solid() );
		morph.set( 0.5F );
		CHECK( lengths_are( dash_of( morph ), { 0.2F, 0.2F } ) );
		morph.set( 1.F );
		CHECK( lengths_are( dash_of( morph ), { 0.2F, 0.4F } ) );
	}
	SECTION( "easing past the end does not make a length negative" )
	{
		const Line a = line_from( { 0.F, 0.F }, { 2.F, 0.F }, {} );
		const Line b = line_from( { 0.F, 1.F }, { 2.F, 1.F }, StrokeDash::dashed( 0.2F, 0.4F ) );
		Morph morph( a, b );
		morph.set( -0.5F );
		CHECK( lengths_are( dash_of( morph ), { 0.2F, 0.F } ) );
	}
	SECTION( "a line becoming a filled shape is a whole line on the way" )
	{
		const Line a = line_from( { -2.F, 0.F }, { 2.F, 0.F }, StrokeDash::dashed( 0.2F, 0.4F ) );
		const Rectangle b( 2.F, 1.F );
		Morph morph( a, b );
		CHECK_FALSE( dash_of( morph ).solid() );
		morph.set( 0.5F );
		CHECK( dash_of( morph ).solid() );
		morph.set( 1.F );
		CHECK( dash_of( morph ).solid() );
	}
	SECTION( "a curve with only a stroke is not closed to do it" )
	{
		Path a;
		a.color.a = 0.F;
		a.stroke_thickness = 0.1F;
		a.commands = { MoveTo{ { -1.F, 0.F } }, LineTo{ { 0.F, 1.F } }, LineTo{ { 1.F, 0.F } } };
		const auto shapes = to_outline( a );
		REQUIRE( shapes.size() == 1 );
		CHECK_FALSE( shapes[0].contours[0].closed );
		a.commands.push_back( Close{} );
		CHECK( to_outline( a )[0].contours[0].closed );
		a.commands.pop_back();
		a.color.a = 1.F; // a fill closes it
		CHECK( to_outline( a )[0].contours[0].closed );
	}
	SECTION( "what is scaled has its dashes scaled" )
	{
		Group a;
		a.scale_x = a.scale_y = 2.F;
		auto &inner = a.add( std::make_unique<Line>() );
		inner = line_from( { 0.F, 0.F }, { 1.F, 0.F }, StrokeDash::dashed( 0.1F, 0.2F ) );
		const Line b = line_from( { 0.F, 1.F }, { 2.F, 1.F }, {} );
		Morph morph( a, b );
		CHECK( lengths_are( dash_of( morph ), { 0.2F, 0.4F } ) );
	}
}

TEST_CASE( "A dashed line that has been drawn on stays dashed", "[dash]" )
{
	Stage stage;
	auto &line = stage.visuals.add( std::make_unique<Line>() );
	line.end = { 2.F, 0.F };
	line.stroke_thickness = 0.1F;
	line.stroke_dash = StrokeDash::dashed( 0.2F, 0.1F );

	Scene scene = draw_on( stage, line, 1s, 0.7F );
	scene.resume();
	scene.resume();
	std::ostringstream during;
	SVGWriter writer( during );
	stage.visuals.accept( writer );
	CHECK( contains( during.str(), "stroke-dasharray=\"0.2 0.1\"" ) );
	CHECK( contains( during.str(), "<path" ) );
	while ( !scene.done() )
	{
		scene.resume();
	}
}

TEST_CASE( "A graph can be dashed, and stays so as it is drawn on", "[dash]" )
{
	Axes axes{ AxesOptions{} };
	GraphOptions options;
	options.dash = StrokeDash::dashed( 0.2F, 0.1F );
	const auto graph = plot( axes, []( double x ) { return x; }, options );
	CHECK( contains( svg_of( *graph ), "stroke-dasharray=\"0.2 0.1\"" ) );
	graph->set_drawn( 0.5F );
	CHECK( contains( svg_of( *graph ), "stroke-dasharray=\"0.2 0.1\"" ) );
}

TEST_CASE( "A connector dashes its line and not its arrow heads", "[dash]" )
{
	Circle a( 0.5F );
	Circle b( 0.5F );
	b.position = { 5.F, 0.F };
	ConnectorOptions options;
	options.dash = StrokeDash::dashed( 0.2F, 0.1F );
	const auto connector = make_connector( a, b, options );
	CHECK( connector->line().stroke_dash == options.dash );
	CHECK( connector->end_tip().stroke_dash.solid() );
	CHECK( contains( svg_of( *connector ), "stroke-dasharray=\"0.2 0.1\"" ) );
}
