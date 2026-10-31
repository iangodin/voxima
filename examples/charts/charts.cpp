// Plots, charts and diagrams: graphs on axes, scatter plots and bar charts, a layered diagram with a callout, and geometry.
//
// Build it (see CMakeLists.txt), then
//
//     voxima --plugin build/charts --script-dir scripts --lang en --list-scenes
//     voxima --plugin build/charts --script-dir scripts --lang en --scene plot --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <code/LineHighlight.h>
#include <core/Animate.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Tween.h>
#include <core/Wait.h>
#include <plot/DrawAxes.h>
#include <plot/Legend.h>
#include <plot/NumberLine.h>
#include <visuals/Brace.h>
#include <visuals/Callout.h>
#include <visuals/CreateList.h>
#include <visuals/Diagram.h>
#include <visuals/Geometry.h>
#include <visuals/WrapText.h>

#include <chrono>
#include <cmath>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
// Axes with a grid, and two graphs drawn on them: a sine wave, then 1/x (in two pieces, either side of 0)
Scene show_plot_with( Stage &stage, AxisPlacement placement )
{
	AxesOptions options;
	options.placement = placement;
	options.x = { -6.0, 6.0 };
	options.y = { -2.5, 2.5 };
	options.width = 12.F;
	options.height = 5.F;
	options.grid = true;
	auto &axes = stage.visuals.add( make_axes( *stage.theme, options ) );
	co_await draw_axes( stage, axes, 2.5s );
	co_await wait_for( stage, 0.3s );

	auto &wave = axes.add_plot( plot( axes, []( double x ) { return std::sin( x ); }, { .name = "sin x" } ) );
	co_await draw( stage, wave, 2s );

	GraphOptions orange;
	orange.color = stage.theme->palette().orange;
	orange.name = "1 / x";
	auto &inverse = axes.add_plot( plot( axes, []( double x ) { return 1.0 / x; }, orange ) );
	co_await draw( stage, inverse, 2s );

	// A key to the two curves
	auto &key = add_legend( axes, make_legend( *stage.theme, { legend_entry( wave ), legend_entry( inverse ) } ), Anchor::TOP_RIGHT );
	co_await fade_in( stage, key, 0.6s );
	co_await wait_for( stage, 2s );
}

// The same, with the axes crossing in the middle
Scene show_plot( Stage &stage, Script & )
{
	co_await show_plot_with( stage, AxisPlacement::ORIGIN );
}

// ... and with the axes along the left and bottom edges
Scene show_plot_edge( Stage &stage, Script & )
{
	co_await show_plot_with( stage, AxisPlacement::EDGE );
}

// Data charts: scatter plots (markers, and a line through them), and bars grouped and stacked
Scene show_charts( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;

	// Points, one series joined by a line, another as crosses
	auto &plot_axes = stage.visuals.add( make_axes( theme, { .x = { 0, 10 }, .y = { 0, 8 }, .width = 7.F, .height = 4.5F, .grid = true, .x_label = "t", .y_label = "v" } ) );
	plot_axes.position = { -3.7F, 0.2F };
	auto &measured = plot_axes.add_plot( scatter( plot_axes, { { 1, 1.5 }, { 2, 2.4 }, { 3.5, 3.0 }, { 5, 5.2 }, { 6.5, 5.6 }, { 8, 7.1 }, { 9.5, 7.4 } }, { .line = JoinStyle::SMOOTH, .name = "measured" } ) );
	auto &noisy = plot_axes.add_plot( scatter( plot_axes, { { 1, 3.0 }, { 2.5, 2.2 }, { 4, 4.6 }, { 5.5, 3.1 }, { 7, 4.4 }, { 9, 5.0 } }, { .marker = MarkerShape::CROSS, .size = 0.14F, .name = "noisy" } ) );
	auto &peaks = plot_axes.add_plot( scatter( plot_axes, { { 2, 6.5 }, { 4, 7.4 }, { 6, 6.0 } }, { .marker = MarkerShape::TRIANGLE, .name = "peaks" } ) );
	add_legend( plot_axes, make_legend( theme, { legend_entry( measured ), legend_entry( noisy ), legend_entry( peaks ) } ), Anchor::BOTTOM_RIGHT );

	// Bars side by side
	auto &bars = stage.visuals.add( make_bar_chart( theme, {
		.categories = { "Q1", "Q2", "Q3", "Q4" },
		.series = { { "north", { 12, 19, 15, 22 } }, { "south", { 9, 14, 20, 17 } } },
		.width = 4.8F, .height = 4.F, .values = Range{ 0, 30 }, .value_labels = true } ) );
	bars.position = { 4.9F, 0.6F };
	add_legend( bars.axes(), make_legend( theme, legend_entries( bars ), { .horizontal = true } ), Anchor::TOP_LEFT );

	auto &title = stage.visuals.add( make_text( theme, "Points and bars", FontStyle{ Intent::TITLE } ) );
	move_to( title, { 0.F, 4.1F }, Anchor::TOP );

	co_await wait_for( stage, 2s );

	// Stacked, going across, in a color a category
	stage.visuals.clear();
	auto &stacked = stage.visuals.add( make_bar_chart( theme, {
		.categories = { "one", "two", "three" },
		.series = { { "a", { 3, 5, 2 } }, { "b", { 2, 1, 4 } }, { "c", { 1, 2, 3 } } },
		.stacked = true, .horizontal = true, .width = 6.F, .height = 3.5F, .value_labels = true } ) );
	stacked.position = { -4.F, 0.F };
	auto &signed_bars = stage.visuals.add( make_bar_chart( theme, {
		.categories = { "a", "b", "c", "d", "e" },
		.series = { { "change", { 4, -2, 3, -5, 1 } } },
		.width = 5.5F, .height = 4.F, .color_by_category = true } ) );
	signed_bars.position = { 4.5F, 0.F };
	co_await wait_for( stage, 2s );
}

// A diagram laid out in layers, a callout pointing into it, a wrapped list, and things spread out evenly
Scene show_diagram( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();
	const FontStyle label{ Intent::LABEL };

	auto tinted = [&]( Color accent )
	{
		return PanelOptions{ .fill = { accent.r, accent.g, accent.b, 0.25F }, .outline = accent, .outline_width = 0.04F };
	};

	auto &flow = stage.visuals.add( make_diagram() );
	flow.add_text_node( theme, "read", "Read input", label, {}, tinted( palette.blue ) );
	flow.add_text_node( theme, "parse", "Parse", label, {}, tinted( palette.green ) );
	flow.add_text_node( theme, "check", "Validate", label, {}, tinted( palette.green ) );
	flow.add_text_node( theme, "run", "Run", label, {}, tinted( palette.orange ) );
	flow.add_text_node( theme, "log", "Write a log", label, {}, tinted( palette.purple ) );
	flow.add_edge( "read", "parse" );
	flow.add_edge( "read", "check" );
	flow.add_edge( "parse", "run" );
	flow.add_edge( "check", "run" );
	flow.add_edge( "run", "log" );
	flow.layout_layers( { .layer_gap = 0.9F } );
	// Off to the left: the nodes moved (not the diagram, so the callout, in the stage's frame, finds them where they are)
	for ( const char *id: { "read", "parse", "check", "run", "log" } )
	{
		flow.node( id ).position = flow.node( id ).position + Point{ -3.5F, 0.F };
	}
	flow.update();

	auto &note = stage.visuals.add( make_callout(
		make_paragraph( theme, "Both checks must finish before anything runs", label, 3.6F ),
		flow.node( "run" ) ) );
	next_to( note.panel(), flow.node( "run" ), Anchor::RIGHT, 1.6F );
	note.update();

	auto &list = stage.visuals.add( make_list( theme, { "Each step is a node", "Arrows say what waits for what", "The layers are worked out for you" }, { .max_width = 5.2F } ) );
	move_to( list, { 4.2F, -2.6F }, Anchor::CENTER );

	// A brace along the list's left, with its label
	stage.visuals.add( make_labeled_brace_beside( theme, list, Anchor::LEFT, "3 ideas", label, 0.25F, { .color = palette.muted } ) );

	// A number line, with a mark and an interval
	auto &numbers = stage.visuals.add( make_number_line( theme, { .range = { 0.0, 10.0 }, .length = 7.F } ) );
	numbers.position = { 3.8F, 1.4F };
	numbers.add_interval( theme, 6.0, 9.0, palette.blue, { .closed_to = false } );
	numbers.add_mark( theme, 2.5, "x", palette.orange );

	// Three things of different sizes, spread evenly along the top
	auto &top = stage.visuals.add( std::make_unique<Group>() );
	for ( const char *word: { "Short", "A longer heading", "Mid" } )
	{
		top.add( make_text( theme, word, FontStyle{ Intent::TITLE } ) );
	}
	arrange_row( top, { .gap = 0.F } );
	{
		int i = 0;
		for ( const auto &child: top )
		{
			child->position = child->position + Point{ i == 0 ? -5.F : ( i == 1 ? 0.F : 5.F ), 0.F };
			++i;
		}
	}
	distribute_row( top );
	move_to( top, { top.position.x(), 4.0F }, Anchor::TOP );
	keep_inside( top, safe_area( stage.camera ) );

	co_await wait_for( stage, 2s );

	// The same kind of thing going across, built up node by node: the boxes come in one after another, then the arrows, then a
	// node is pointed out and moved, the arrows following it
	stage.visuals.clear();
	stage.updaters.clear();
	auto &pipeline = stage.visuals.add( make_diagram() );
	for ( const auto &[id, name, accent]: { std::tuple{ "in", "Input", palette.blue }, std::tuple{ "clean", "Clean", palette.green }, std::tuple{ "model", "Model", palette.orange }, std::tuple{ "out", "Report", palette.purple } } )
	{
		pipeline.add_text_node( theme, id, name, label, {}, tinted( accent ) );
	}
	pipeline.add_edge( "in", "clean" );
	pipeline.add_edge( "clean", "model" );
	pipeline.add_edge( "model", "out" );
	pipeline.layout_layers( { .direction = Direction::RIGHT, .layer_gap = 1.4F } );
	auto tie = follow( stage, pipeline );

	// (nodes hidden until their turn: the arrows are drawn over the space between)
	for ( size_t i = 0; i < pipeline.edge_count(); ++i )
	{
		pipeline.edge( i ).color.a = 0.F;
		pipeline.edge( i ).stroke_color.a = 0.F;
	}
	std::vector<Visual *> nodes;
	for ( const char *id: { "in", "clean", "model", "out" } )
	{
		nodes.push_back( &pipeline.node( id ) );
	}
	co_await stagger( stage, nodes, 0.25s, [&]( Visual &node ) { return grow_in( stage, node, 0.5s ); } );
	std::vector<Visual *> arrows;
	for ( size_t i = 0; i < pipeline.edge_count(); ++i )
	{
		arrows.push_back( &pipeline.edge( i ) );
	}
	for ( Visual *arrow: arrows )
	{
		arrow->color.a = 1.F;
		arrow->stroke_color.a = 1.F;
	}
	co_await stagger( stage, arrows, 0.2s, [&]( Visual &arrow ) { return fade_in( stage, arrow, 0.4s ); } );
	co_await circumscribe( stage, pipeline.node( "model" ) );
	co_await move_by( stage, pipeline.node( "model" ), { 0.F, 1.8F }, 0.8s, ease_cubic_in_out );
	co_await move_by( stage, pipeline.node( "model" ), { 0.F, -1.8F }, 0.8s, ease_cubic_in_out );
	co_await wait_for( stage, 3s );
}

// A triangle with named corners, a right angle and another marked
Scene show_geometry( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();
	const Point a{ -4.F, -2.F };
	const Point b{ 2.F, -2.F };
	const Point c{ 2.F, 2.F };

	stage.visuals.add( make_polygon( theme, { a, b, c }, { .fill = Color{ palette.blue.r, palette.blue.g, palette.blue.b, 0.2F }, .labels = { "A", "B", "C" }, .dots = true } ) );
	stage.visuals.add( make_angle( theme, b, a, c, { .radius = 0.5F } ) );
	stage.visuals.add( make_angle( theme, a, b, c, { .radius = 1.2F, .color = palette.orange, .label = "θ" } ) );
	stage.visuals.add( make_labeled_brace( theme, a, b, "adjacent", FontStyle{ Intent::LABEL }, { .color = palette.muted, .left = false } ) );

	co_await wait_for( stage, 2s );
}
}

REGISTER_SCENE( "plot", "Axes crossing in the middle, and graphs drawn on them", show_plot );
REGISTER_SCENE( "plot-edge", "The same, with the axes along the left and bottom", show_plot_edge );
REGISTER_SCENE( "charts", "Scatter plots and bar charts: grouped, stacked, horizontal, with negatives", show_charts );
REGISTER_SCENE( "diagram", "A diagram laid out in layers, a callout, a wrapped list, and evenly spread headings", show_diagram );
REGISTER_SCENE( "geometry", "A triangle with named corners and marked angles", show_geometry );
