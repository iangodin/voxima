# Animation verbs and the camera

These are the common things to do to a visual. Each is a scene you `co_await` (or run several at once with
`choreograph`). The code on this page is from the `animation` example (`examples/animation`), which is built with Voxima. Each takes the visual **as it is now** as its resting state and leaves it there, except the ones that
take it away. All take a length in seconds and, where it makes sense, an [ease](#easing).

```cpp
--8<-- "examples/animation/animation.cpp:verbs"
```

| Verb | Does |
|---|---|
| `fade_in`, `fade_out` | From nothing to how it is, and back (it is left there, invisible: remove it if it should go) |
| `fade_to( stage, v, opacity )` | To an opacity, and it stays there: dim with `0.3F`, bring back with `1.F` (`Visual::opacity`) |
| `grow_in`, `shrink_out` | Grow from its own center, fading in; the reverse |
| `slide_in( stage, v, offset, ...)`, `slide_out` | Come in from `offset` away (stage units), fading in; go out to it |
| `move_by( stage, v, offset, ...)` | Move by an offset |
| `move_along( stage, v, path, ... , turn )` | Follow a `Path`; with `turn`, face the way it is going |
| `turn_about( stage, v, center, degrees, ...)` | Go round a point, turning with it |
| `indicate( stage, v, factor, ...)` | Swell by `factor` (1.2) and back |
| `draw_on( stage, v, duration )` | Trace the outlines with a line, then fade the fill in: for text, shapes and paths |
| `circumscribe`, `flash` | Draw a line round a visual with a pen; a ring that spreads from its edge. Options: `PointOutOptions` |
| `stagger( stage, visuals, delay, verb )` | Run a verb on each of several visuals, each starting `delay` after the last |

`stagger` takes the verb as a function from a visual to a `Scene`, and `children_of( group )` lists a group's
direct children:

```cpp
--8<-- "examples/animation/animation.cpp:stagger"
```

`draw_on`, `circumscribe` and `flash` draw their outlines in the group the visual is a child of (give it as the
last argument if that is not the stage), so the visual has to be a direct child of that group.

## Easing

An **ease** turns how far along something is, `t` from 0 to 1, into how far along it *looks*: slow at first,
overshooting, bouncing. It is a plain function of `t`, so any moment can be worked out in any order.

| Ease | Feel |
|---|---|
| `ease_linear` | Steady |
| `ease_cubic_in`, `ease_cubic_out`, `ease_cubic_in_out` | Smooth: slow start, slow end, or both |
| `ease_back_in`, `ease_back_out`, `ease_back_in_out` | Pull back first, or overshoot a little |
| `ease_elastic_in`, `ease_elastic_out`, `ease_elastic_in_out` | Spring |

Pass one to a verb or to [`tween`](../concepts/coroutines.md#running-something-every-frame). Any function from
float to float works (`EaseFunction` is `std::function<float( float )>`), so writing your own is one line:

```cpp
auto smooth = []( float t ) { return t * t * ( 3.F - 2.F * t ); };
```

The back and elastic eases are also types, with parameters for how far and how quick the swings are (see
`core/EaseBack.h`, `core/EaseElastic.h`).

## Tying things together

A label that follows a moving thing, a line between two moving boxes: use
[updaters](../concepts/coroutines.md#running-something-every-frame) (`stage.updaters.add`). A `Connector` has a
ready-made one, `follow`:

```cpp
--8<-- "examples/animation/animation.cpp:connector"
```

## The camera

`stage.camera` says what part of the stage is seen: its `center` (a point, stage units) and `zoom` (1 is the
whole 16-unit-wide picture; 2 shows half of that, so things look twice as big, lines included).

```cpp
--8<-- "examples/animation/animation.cpp:camera"
```

`camera_frame( stage, some_visual, 1s )` fits a visual (or an AABB) to the picture instead. The zoom moves evenly in its logarithm and the center follows, so zooming in on something looks like moving in
on it. `camera_to` and `camera_frame` default to `ease_cubic_in_out`. `view_framing( camera, box, margin )` works
out the view that fits a box, if you want to use it yourself.

The camera is at its default at the start of every scene.
