# Morphing

A **morph** turns one visual into another: a circle into a square, a letter into a different letter, a formula
into the next step of its derivation, a sentence into an edited sentence. Both visuals are flattened to
outlines, their parts are paired up and cut to match, and the shape is blended from one to the other.

The code on this page is from the `morphing` and `narration` examples (`examples/morphing`, `examples/narration`), which are built with Voxima.

```cpp
--8<-- "examples/morphing/morphing.cpp:morph"
```

What happens to the visuals:

- `from` must be a **direct child of `stage.visuals`**, or of the group given as `MorphOptions::parent`, with `to` made
  for that group's frame. It is taken off for the morph and **dies with it**: do not use its reference afterwards.
- `to` takes its place when the morph finishes, so the last frame is the real thing, not a flattened copy. Keep
  a raw pointer to it *before* you move it in if you will animate it next:

```cpp
--8<-- "examples/morphing/morphing.cpp:pointer"
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
--8<-- "examples/morphing/morphing.cpp:tags"
```

Here the `a` and `b` slide into place; everything else matches by glyph, and the parentheses fade.

## Options

```cpp
--8<-- "examples/morphing/morphing.cpp:options"
```

`stagger` makes a word change letter by letter, left to right. `parent` is the group the morph happens in, if that is not
the stage.

Fills, strokes, [dashes](visuals-layout.md#dashed-lines) and [gradients](visuals-layout.md#gradients) are blended as well as the
shapes. What has an `opacity` has it put into its colors for the morph, so what overlaps inside a dimmed group can show through
until the morph ends.

## Driving a morph yourself

`morph_driven` takes the thing that decides *when*: a scene that calls `morph.set( t )` as it goes. That is how a
morph is tied to narration:

```cpp
--8<-- "examples/narration/narration.cpp:driven"
```

`set( t )` blends from `from` (0) to `to` (1); values outside that overshoot, which is what back and elastic eases
do. A picture (`Image`) is not an outline and cannot blend; see [Visuals](visuals-layout.md#pictures).
