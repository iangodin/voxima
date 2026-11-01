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
