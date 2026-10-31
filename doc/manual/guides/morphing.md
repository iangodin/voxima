# Morphing

A **morph** turns one visual into another: a circle into a square, a letter into a different letter, a formula
into the next step of its derivation, a sentence into an edited sentence. Both visuals are flattened to
outlines, their parts are paired up and cut to match, and the shape is blended from one to the other.

```cpp
auto square = std::make_unique<Rectangle>( 0.6F, 0.6F, theme.palette().green );
square->position = circle.position;
co_await morph( stage, circle, std::move( square ), 1.5s, ease_cubic_in_out );
```

What happens to the visuals:

- `from` must be a **direct child of `stage.visuals`**. It is taken off the stage for the morph and **dies with it**:
  do not use its reference afterwards.
- `to` takes its place when the morph finishes, so the last frame is the real thing, not a flattened copy. Keep
  a raw pointer to it *before* you move it in if you will animate it next:

```cpp
Visual *next = square.get();
co_await morph( stage, *shown, std::move( square ), 1s, ease_cubic_in_out );
shown = next;
```

## How parts are matched

Each *leaf* (a glyph, a shape) of the two visuals is paired with one of the other. By default (`LeafMatch::AUTO`):

1. words of laid-out text, kept in reading order;
2. leaves with the **same tag**;
3. identical glyphs, in reading order;
4. what is left, by closeness (only leaves near each other and of similar size).

A leaf with no partner fades out or in. A pair that is the same shape only moved, turned or scaled (the head of
an arrow that turns) is carried there whole.

So for a derivation, **tag the parts that should be followed** from one step to the next:

```cpp
using namespace math;
const Expr a = tag( "a", text( "a" ) );
const Expr b = tag( "b", text( "b" ) );

auto first  = superscript( parens( row( { ord( a ), bin( "+" ), ord( b ) } ) ), text( "2" ) );
auto second = row( { ord( superscript( a, text( "2" ) ) ), bin( "+" ), ord( text( "2ab" ) ),
                     bin( "+" ), ord( superscript( b, text( "2" ) ) ) } );
```

Here the `a` and `b` slide into place; everything else matches by glyph, and the parentheses fade.

## Options

```cpp
MorphOptions options;
options.leaves = LeafMatch::BY_WORD;   // BY_WORD, BY_TAG, ... instead of AUTO
options.stagger = 0.4F;                // start the parts one after another (0 to 0.95 of the time)
options.part_ease = smooth;            // an ease for each part's own progress
co_await morph( stage, *shown, std::move( target ), 1.7s, ease_cubic_in_out, options );
```

`stagger` makes a word change letter by letter, left to right.

## Driving a morph yourself

`morph_driven` takes the thing that decides *when*: a scene that calls `morph.set( t )` as it goes. That is how a
morph is tied to narration:

```cpp
co_await morph_driven( stage, *shown, std::move( target ), MorphOptions{},
	[&]( Morph &m ) { return narration.track_markers( "a", "b", [&m]( float t ) { m.set( t ); } ); } );
```

`set( t )` blends from `from` (0) to `to` (1); values outside that overshoot, which is what back and elastic eases
do. A picture (`Image`) is not an outline and cannot blend; see [Visuals](visuals-layout.md#pictures).
