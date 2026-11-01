# Visuals and layout

The code on this page is from the `layout` example (`examples/layout`), which is built with Voxima.

## Shapes

```cpp
--8<-- "examples/layout/layout.cpp:shapes"
```

Every visual has the same properties:

| Property | Meaning |
|---|---|
| `position` | Where its origin is, in its parent's frame (stage units, y up) |
| `rotation`, `scale_x`, `scale_y` | Turned and scaled about its origin |
| `color` | Fill color (alpha makes it see-through) |
| `fill_gradient` | A fill that changes color across it, instead (see [Gradients](#gradients)) |
| `stroke_thickness`, `stroke_color`, `stroke_dash` | Its outline (see [Dashed lines](#dashed-lines)) |
| `tags` | Names for finding it again (see [Tags](#tags)) |

`Line` has a `start` and `end`; a `Path` is made of `MoveTo` and `CubicTo` commands; `Group` holds other visuals.
A group's position, rotation, scale and **color multiply down into its children**, so fading a group fades
everything in it.

## Groups

```cpp
--8<-- "examples/layout/layout.cpp:groups"
```

Iterate a group with a range-for; `size()`, `clear()`, `remove( &child )` (hands the child back) and
`replace( &old, new )` work on its direct children.

## Drawing order

A group draws its children in order, so what comes later is in front. To put something elsewhere in that order, use
`insert( index, visual )` (0 is the back) instead of `add`, or move a child that is already there:

```cpp
--8<-- "examples/layout/layout.cpp:order"
```

`to_front`, `to_back`, `raise` and `lower` take a direct child, and return false if it is not one. All of these stay
in front of the stage's background.

## Dashed lines

Any stroke can be dashed or dotted, with a `StrokeDash`:

```cpp
--8<-- "examples/layout/layout.cpp:dashes"
```

A `pattern` is lengths on and off in turn, in the visual's own units, so it scales with the visual just as its stroke
does (and stretches with it under an uneven scale). `offset` starts it further along; tween it for dashes that march.
Each piece of a path (each `MoveTo`) starts the pattern again. A pattern under half a pixel is drawn as a whole line.

`ConnectorOptions::dash` dashes a connector's line and not its arrow heads, and `GraphOptions::dash` a graph's curve.
Dashes stay as they are in `draw_on`, and a morph blends them: between two patterns, or with the gaps opening
from a whole line. The exception is a line that becomes a filled shape: it is drawn as a whole line on the way, so the
gaps close as soon as it starts.

## Gradients

Any shape can be filled with a linear or radial gradient instead of a plain color:

```cpp
--8<-- "examples/layout/layout.cpp:gradients"
```

A gradient is placed in the visual's own coordinates, so it moves, turns and scales with the visual. `Gradient::across`
fits a linear one to a box at an angle (0 goes to the right, 90 up); `box_of` gives that box only while the visual is
still at its origin, so use it before placing the visual. Stops are at offsets from 0 to 1. The visual's
`color` tints the gradient and its alpha fades it, and white in a stop is the theme's text color, as everywhere.

**Text** has a gradient per glyph, not one across a word: `fill_each_with_gradient` gives each shape below a visual its
own, across that shape.

Colors blend as they are written, in sRGB and not in linear light; add stops to follow any other curve. A frame has 8
bits a channel and gradients are not dithered, so a long, dark ramp shows bands (from 0 to 16 across 1600 pixels: 17
bands, each about 94 pixels wide). Between stops with different alpha, Voxima's renderer blends colors
premultiplied, so a color fading out does not pick up the hue of the transparent stop, as it would in a browser
looking at the same SVG. A gradient that goes nowhere (its ends the same, or a radius of 0) is drawn in its last color.

**A morph** blends gradients: a plain fill and a gradient turn into one another, and two of a kind blend their stops and
their places. A linear and a radial one cannot become one another, so they pass through a plain color, the average
of the two:

```cpp
--8<-- "examples/layout/layout.cpp:gradient-morph"
```

## Placing things against each other

Hard-coded coordinates are fine for a couple of things. For anything more, place visuals **relative to each
other** (`visuals/Placement.h`), so a change in size or text does not make things overlap. The boxes used are
the ones layout uses: for text, the advance widths and line height, not just the ink.

```cpp
--8<-- "examples/layout/layout.cpp:placement"
```

`Anchor` is `CENTER`, `LEFT`, `RIGHT`, `TOP`, `BOTTOM` or a corner (`TOP_LEFT`, ...). Visuals placed against each
other must be in the same group (or both directly on the stage).

**Rows, columns and grids** arrange the children of a group:

```cpp
--8<-- "examples/layout/layout.cpp:arrange"
```

`Align` (across the direction of the row) is `START`, `CENTER`, `END`, or `BASELINE`, which lines up the
baselines of text of any size.

**Keeping things on screen:**

```cpp
--8<-- "examples/layout/layout.cpp:keep-inside"
```

`safe_area` is the picture's frame, at zoom 1, pulled in by a margin (0.5 units by default).

`box_of( visual )` and `point_at( visual, anchor )` give a visual's box and one of its points, in its parent's
frame, for your own placement.

## Panels, lists and tables

A **panel** is a rounded box fitted round something:

```cpp
--8<-- "examples/layout/layout.cpp:panel"
```

`PanelOptions` sets the `padding`, corner `radius`, `fill`, `outline` and `outline_width`, and an optional
`shadow`. The default fill and outline are faint tints of the text color, so a panel suits any palette.

A **list** is a bulleted or numbered column:

```cpp
--8<-- "examples/layout/layout.cpp:list"
```

Long items wrap if you set `max_width`; every row is spaced evenly.

A **table** has rows of cells, a header, and optional rules, stripes and wrapping:

```cpp
--8<-- "examples/layout/layout.cpp:table"
```

Cells can also be visuals (a formula, a picture); then give `make_table` a vector of rows of
`std::unique_ptr<Visual>`.

## Pictures

An `Image` shows a picture file: set its `path`, `half_width` and `half_height`. A picture is not an outline, so it
[morphs](morphing.md) only into the same picture (moving to where that is); otherwise it cross-fades.

## Tags

Tag a visual to find it again after it is built into something bigger:

```cpp
--8<-- "examples/layout/layout.cpp:tags"
```

A path of names looks inside each match in turn (`find_tag( built, "fraction1", "bar" )`). Math formulas and
other composites name their own parts too (`numerator`, `denominator`, `bar`, ...), listed in `visuals/Tags.h`.
Tags are also what [morphing](morphing.md) uses to follow a part from one visual to the next.
