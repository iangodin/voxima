# Charts and diagrams

The code on this page is from the `charts` example (`examples/charts`), which is built with Voxima.

## Axes and graphs

```cpp
--8<-- "examples/charts/charts.cpp:axes"
```

Other options: `x_label`, `y_label`, `numbers`, `arrows`, `grid_x`, `grid_y`, custom `x_tick_labels` /
`y_tick_labels` (a tick with its own text: a year, "π"), and colors and thicknesses.

Add **graphs of functions** with `plot`, and draw them on:

```cpp
--8<-- "examples/charts/charts.cpp:graphs"
```

A function that blows up (like 1/x) is drawn in pieces on either side of the break. `GraphOptions` also sets the
`x` range to plot, the number of `samples`, and the `thickness`. Each new graph takes the next color of the palette's
series (blue, orange, green, purple, ...) unless you give one.

`axes.at( x, y )` converts a point of the graph to a stage point: put things on the graph, or aim the camera at a
peak with it.

A graph's curve can be dashed with `GraphOptions::dash` (see [Dashed lines](visuals-layout.md#dashed-lines)).

## Scatter plots

```cpp
--8<-- "examples/charts/charts.cpp:scatter"
```

`marker` is `CIRCLE`, `SQUARE`, `DIAMOND`, `TRIANGLE`, `CROSS` or `PLUS`; `line` joins the points: `NONE`, `STRAIGHT`
or `SMOOTH` (a curve through them, which can overshoot between points far apart).

## Bar charts

```cpp
--8<-- "examples/charts/charts.cpp:bars"
```

Grouped by default; `.stacked = true` stacks them, `.horizontal = true` turns the chart on its side,
`.color_by_category = true` gives each category its own color (for a single series), and negative values go
below the axis. `.value_labels` writes the numbers on the bars.

## Legends

```cpp
--8<-- "examples/charts/charts.cpp:legend"
```

Name your plots and series (`.name = ...`): the legend takes names and colors from them.

## Number lines

```cpp
--8<-- "examples/charts/charts.cpp:number-line"
```

## Diagrams

A `Diagram` is a set of labeled boxes (nodes) joined by arrows (edges), laid out in layers for you:

```cpp
--8<-- "examples/charts/charts.cpp:diagram"
```

Nodes are panels (`flow.node( "run" )`); give `add_text_node` a `PanelOptions` to tint one. `add_node` takes any
visual as the content. Edges are connectors: `flow.edge( i )`, `flow.edge_count()`. If you move a node, call
`flow.update()` (or tie it with `follow( stage, flow )`) and the arrows follow.

Build a diagram up on screen by staggering nodes then edges:

```cpp
--8<-- "examples/charts/charts.cpp:diagram-stagger"
```

## Connectors, callouts and braces

```cpp
--8<-- "examples/animation/animation.cpp:connector"
--8<-- "examples/charts/charts.cpp:callout"
--8<-- "examples/charts/charts.cpp:brace"
```

A callout is a panel with a pointer to a target; after moving either, call `note.update()`. Braces come in
plain (`make_brace( from, to )`, `make_brace_beside( visual, side )`) and labeled forms.

A connector's line can be dashed with `ConnectorOptions::dash` (see [Dashed lines](visuals-layout.md#dashed-lines)).

## Geometry

```cpp
--8<-- "examples/charts/charts.cpp:geometry"
```

`make_polygon` takes the corners; `fill`, `labels` and `dots` are in `PolygonOptions`. `make_angle` marks the angle
at a vertex between two points, as an arc (a right angle gets a little square, unless `square_for_right` is off).
