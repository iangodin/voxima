# Visuals and layout

## Shapes

```cpp
auto circle = std::make_unique<Circle>( 0.7F, theme.palette().blue );   // radius, color
auto &dot = stage.visuals.add( std::move( circle ) );
dot.position = { -3.F, 0.F };

auto &box = stage.visuals.add( std::make_unique<Rectangle>( 1.F, 0.5F, theme.palette().green ) );  // half width, half height
```

Every visual has the same properties:

| Property | Meaning |
|---|---|
| `position` | Where its origin is, in its parent's frame (stage units, y up) |
| `rotation`, `scale_x`, `scale_y` | Turned and scaled about its origin |
| `color` | Fill color (alpha makes it see-through) |
| `stroke_thickness`, `stroke_color` | Its outline |
| `tags` | Names for finding it again (see [Tags](#tags)) |

`Line` has a `start` and `end`; a `Path` is made of `MoveTo` and `CubicTo` commands; `Group` holds other visuals.
A group's position, rotation, scale and **color multiply down into its children**, so fading a group fades
everything in it.

## Groups

```cpp
auto &row = stage.visuals.add( std::make_unique<Group>() );
row.add( std::make_unique<Circle>( 0.4F ) );
row.add( std::make_unique<Circle>( 0.4F ) );
```

Iterate a group with a range-for; `size()`, `clear()`, `remove( &child )` (hands the child back) and
`replace( &old, new )` work on its direct children.

## Placing things against each other

Hard-coded coordinates are fine for a couple of things. For anything more, place visuals **relative to each
other** (`visuals/Placement.h`), so a change in size or text does not make things overlap. The boxes used are
the ones layout uses: for text, the advance widths and line height, not just the ink.

```cpp
move_to( title, { 0.F, 3.5F }, Anchor::TOP );          // its top edge at y = 3.5
next_to( label, circle, Anchor::BOTTOM );              // under it, a gap between
next_to( panel, other, Anchor::RIGHT, 1.0F );          // to its right, 1 unit apart
```

`Anchor` is `CENTER`, `LEFT`, `RIGHT`, `TOP`, `BOTTOM` or a corner (`TOP_LEFT`, ...). Visuals placed against each
other must be in the same group (or both directly on the stage).

**Rows, columns and grids** arrange the children of a group:

```cpp
arrange_row( group, { .gap = 0.5F } );                     // left to right, centered on the origin
arrange_column( group, { .gap = 0.3F, .align = Align::START } );
arrange_grid( group, 3, { .column_gap = 0.4F, .row_gap = 0.2F } );   // rows of 3
distribute_row( group );                                   // equal spaces between, outer two stay put
align_edges( group, Anchor::LEFT );                        // line up on an edge
```

`Align` (across the direction of the row) is `START`, `CENTER`, `END`, or `BASELINE`, which lines up the
baselines of text of any size.

**Keeping things on screen:**

```cpp
keep_inside( visual, safe_area( stage.camera ) );   // moved the least it takes
fit_inside( visual, safe_area( stage.camera ) );    // and scaled down if it is too big
```

`safe_area` is the picture's frame, at zoom 1, pulled in by a margin (0.5 units by default).

`box_of( visual )` and `point_at( visual, anchor )` give a visual's box and one of its points, in its parent's
frame, for your own placement.

## Panels, lists and tables

A **panel** is a rounded box fitted round something:

```cpp
auto &panel = stage.visuals.add( make_panel( make_text( theme, "Theorem", style ), { .padding = 0.5F } ) );
```

`PanelOptions` sets the `padding`, corner `radius`, `fill`, `outline` and `outline_width`, and an optional
`shadow`. The default fill and outline are faint tints of the text color, so a panel suits any palette.

A **list** is a bulleted or numbered column:

```cpp
auto &list = stage.visuals.add( make_list( theme, { "first", "second", "third" }, { .numbered = true } ) );
list.row( 1 );    // each row is a visual of its own, to show or light on its own
```

Long items wrap if you set `max_width`; every row is spaced evenly.

A **table** has rows of cells, a header, and optional rules, stripes and wrapping:

```cpp
const std::vector<std::vector<std::string>> rows = {
	{ "Name", "Scale", "Use" },
	{ "tiny", "0.5", "footnotes" },
	{ "huge", "2.0", "titles" } };
auto &table = stage.visuals.add( make_table( theme, rows, { .striped = true, .max_width = 10.F } ) );
table.cell( 1, 2 );   // a cell is a visual
```

Cells can also be visuals (a formula, a picture); then give `make_table` a vector of rows of
`std::unique_ptr<Visual>`.

## Pictures

An `Image` shows a picture file: set its `path`, `half_width` and `half_height`. A picture is not an outline, so it
[morphs](morphing.md) only into the same picture (moving to where that is); otherwise it cross-fades.

## Tags

Tag a visual to find it again after it is built into something bigger:

```cpp
tag_visual( shape, "hero" );
Visual *found = find_tag( stage.visuals, "hero" );
```

A path of names looks inside each match in turn (`find_tag( built, "fraction1", "bar" )`). Math formulas and
other composites name their own parts too (`numerator`, `denominator`, `bar`, ...), listed in `visuals/Tags.h`.
Tags are also what [morphing](morphing.md) uses to follow a part from one visual to the next.
