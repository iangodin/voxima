# Animation and coroutines

A scene is a C++20 coroutine. It runs a little each frame and **pauses at every `co_await`**, so you write an
animation as ordinary top-to-bottom code instead of a state machine:

```cpp
--8<-- "examples/coroutines/coroutines.cpp:intro"
```

The code on this page is from the `coroutines` example (`examples/coroutines`), which is built with Voxima.

A `Scene` is the coroutine type (`Scene` is an alias of `Coroutine`). Everything you can `co_await` in one is a
`Scene` too: the animation verbs, `wait_for`, narration, and other scenes.

## Waiting

```cpp
--8<-- "examples/coroutines/coroutines.cpp:pauses"
```

## Doing things at the same time

`choreograph` runs several animations together and finishes when the **last** one does:

```cpp
--8<-- "examples/coroutines/coroutines.cpp:choreograph"
```

When the number of animations is only known at run time, collect them in a `std::vector<Coroutine>` and use
`choreograph_all`:

```cpp
--8<-- "examples/coroutines/coroutines.cpp:choreograph-all"
```

To start things one after another with an overlap, use `stagger` (see [Animation verbs](../guides/animation.md)).

## Running something every frame

For your own animations, **`tween`** calls a function once per frame with how far along it is, 0 to 1, shaped by
an [ease](../guides/animation.md#easing):

```cpp
--8<-- "examples/coroutines/coroutines.cpp:tween"
```

`tween` always makes at least two frames, a first (`t = 0`) and a last (`t = 1`, exactly).

To keep **one thing tied to another** for as long as you like, add an *updater*. It runs every frame, after the
scene has done its step and before the picture is drawn:

```cpp
--8<-- "examples/coroutines/coroutines.cpp:updater"
```

The returned handle ends the updater when it is destroyed; call `tie.release()` to let it run to the end of the
scene instead. Updaters are cleared when a scene starts. If you remove a visual an updater uses, end the updater
first.

## Lifetimes

An animation refers to the visual it works on, by reference. The visual **must stay on the stage until the
animation is over**. In particular, do not `clear()` the stage or morph a visual away while an animation that
uses it is still running in a `choreograph`.

## When something goes wrong

An exception in a scene goes to what is waiting for it, as if the scene were a function: through the `co_await` and any
`choreograph`, so a scene can catch it there and carry on. One that nothing catches ends the render with its message and a status of
1, and in a preview is shown in the window's title, for you to fix and reload.

```cpp
try
{
    co_await risky_scene( stage );
}
catch ( const std::exception &error )
{
    std::cerr << "skipping it: " << error.what() << '\n';
}
```

## Frame rate and length

Nothing in a scene depends on the frame rate: times are in seconds, rounded to the nearest whole frame. A time
that is not a whole number of frames (half a second at 25 fps) is a frame more or less. The render's length is
however long the scene takes, including the narration it waits for.
