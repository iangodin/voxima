# Charts and diagrams

## Axes and graphs

```cpp
AxesOptions options;
options.x = { -6.0, 6.0 };          // the range shown
options.y = { -2.5, 2.5 };
options.width = 12.F;               // its size on the stage
options.height = 5.F;
options.grid = true;
options.placement = AxisPlacement::ORIGIN;     // crossing in the middle; EDGE puts them along the left and bottom
auto &axes = stage.visuals.add( make_axes( theme, options ) );

co_await draw_axes( stage, axes, 2.5s );       // the axes draw themselves
```

Other options: `x_label`, `y_label`, `numbers`, `arrows`, `grid_x`, `grid_y`, custom `x_tick_labels` /
`y_tick_labels` (a tick with its own text: a year, "π"), and colors and thicknesses.

Add **graphs of functions** with `plot`, and draw them on:

```cpp
auto &wave = axes.add_plot( plot( axes, []( double x ) { return std::sin( x ); }, { .name = "sin x" } ) );
co_await draw( stage, wave, 2s );

GraphOptions orange;
orange.color = theme.palette().orange;
orange.name = "1 / x";
auto &inverse = axes.add_plot( plot( axes, []( double x ) { return 1.0 / x; }, orange ) );
```

A function that blows up (like 1/x) is drawn in pieces on either side of the break. `GraphOptions` also sets the
`x` range to plot, the number of `samples`, and the `thickness`. Each new graph takes the next color of the palette's
series (blue, orange, green, purple, ...) unless you give one.

`axes.at( x, y )` converts a point of the graph to a stage point: put things on the graph, or aim the camera at a
peak with it.

## Scatter plots

```cpp
auto &measured = axes.add_plot( scatter( axes, { { 1, 1.5 }, { 2, 2.4 }, { 3.5, 3.0 } },
	{ .line = JoinStyle::SMOOTH, .name = "measured" } ) );
auto &noisy = axes.add_plot( scatter( axes, points, { .marker = MarkerShape::CROSS, .size = 0.14F } ) );
```

`marker` is `CIRCLE`, `SQUARE`, `DIAMOND`, `TRIANGLE`, `CROSS` or `PLUS`; `line` joins the points: `NONE`, `STRAIGHT`
or `SMOOTH` (a curve through them, which can overshoot between points far apart).

## Bar charts

```cpp
auto &bars = stage.visuals.add( make_bar_chart( theme, {
	.categories = { "Q1", "Q2", "Q3", "Q4" },
	.series = { { "north", { 12, 19, 15, 22 } }, { "south", { 9, 14, 20, 17 } } },
	.width = 4.8F, .height = 4.F, .values = Range{ 0, 30 }, .value_labels = true } ) );
```

Grouped by default; `.stacked = true` stacks them, `.horizontal = true` turns the chart on its side,
`.color_by_category = true` gives each category its own color (for a single series), and negative values go
below the axis. `.value_labels` writes the numbers on the bars.

## Legends

```cpp
auto &key = add_legend( axes, make_legend( theme, { legend_entry( wave ), legend_entry( inverse ) } ), Anchor::TOP_RIGHT );
add_legend( bars.axes(), make_legend( theme, legend_entries( bars ), { .horizontal = true } ), Anchor::TOP_LEFT );
```

Name your plots and series (`.name = ...`): the legend takes names and colors from them.

## Number lines

```cpp
auto &numbers = stage.visuals.add( make_number_line( theme, { .range = { 0.0, 10.0 }, .length = 7.F } ) );
numbers.add_interval( theme, 6.0, 9.0, theme.palette().blue, { .closed_to = false } );
numbers.add_mark( theme, 2.5, "x", theme.palette().orange );
```

## Diagrams

A `Diagram` is a set of labeled boxes (nodes) joined by arrows (edges), laid out in layers for you:

```cpp
auto &flow = stage.visuals.add( make_diagram() );
flow.add_text_node( theme, "read",  "Read input", FontStyle{ Intent::LABEL } );
flow.add_text_node( theme, "parse", "Parse",      FontStyle{ Intent::LABEL } );
flow.add_text_node( theme, "run",   "Run",        FontStyle{ Intent::LABEL } );
flow.add_edge( "read", "parse" );
flow.add_edge( "parse", "run" );
flow.layout_layers( { .direction = Direction::RIGHT, .layer_gap = 1.4F } );   // or Direction::DOWN
```

Nodes are panels (`flow.node( "run" )`); give `add_text_node` a `PanelOptions` to tint one. `add_node` takes any
visual as the content. Edges are connectors: `flow.edge( i )`, `flow.edge_count()`. If you move a node, call
`flow.update()` (or tie it with `follow( stage, flow )`) and the arrows follow.

Build a diagram up on screen by staggering nodes then edges:

```cpp
co_await stagger( stage, nodes, 0.25s, [&]( Visual &n ) { return grow_in( stage, n, 0.5s ); } );
```

## Connectors, callouts and braces

```cpp
auto &line = stage.visuals.add( make_connector( a, b, { .gap = 0.1F } ) );     // an arrow between two visuals
auto &note = stage.visuals.add( make_callout( make_paragraph( theme, "the peak", label, 3.6F ), target ) );
stage.visuals.add( make_labeled_brace_beside( theme, list, Anchor::LEFT, "3 ideas", label ) );
```

A callout is a panel with a pointer to a target; after moving either, call `note.update()`. Braces come in
plain (`make_brace( from, to )`, `make_brace_beside( visual, side )`) and labeled forms.

## Geometry

```cpp
stage.visuals.add( make_polygon( theme, { a, b, c }, { .labels = { "A", "B", "C" }, .dots = true } ) );
stage.visuals.add( make_angle( theme, b, a, c, { .radius = 0.5F } ) );                   // a right-angle mark
stage.visuals.add( make_angle( theme, a, b, c, { .radius = 1.2F, .label = "θ" } ) );
stage.visuals.add( make_labeled_brace( theme, a, b, "adjacent", FontStyle{ Intent::LABEL } ) );
```

`make_polygon` takes the corners; `fill`, `labels` and `dots` are in `PolygonOptions`. `make_angle` marks the angle
at a vertex between two points, as an arc (a right angle gets a little square, unless `square_for_right` is off).
