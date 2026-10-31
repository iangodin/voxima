# Animation and coroutines

A scene is a C++20 coroutine. It runs a little each frame and **pauses at every `co_await`**, so you write an
animation as ordinary top-to-bottom code instead of a state machine:

```cpp
Scene intro( Stage &stage, Script &script )
{
	auto &title = stage.visuals.add( make_text( *stage.theme, "Hello", FontStyle{ Intent::TITLE } ) );
	co_await fade_in( stage, title, 0.5s );   // frames pass here
	co_await wait_for( stage, 1s );
	co_await fade_out( stage, title, 0.5s );
}
```

A `Scene` is the coroutine type (`Scene` is an alias of `Coroutine`). Everything you can `co_await` in one is a
`Scene` too: the animation verbs, `wait_for`, narration, and other scenes.

## Waiting

```cpp
co_await wait_for( stage, 1.5s );     // a pause
co_await wait_frames( 10 );           // exactly 10 frames, whatever the frame rate
co_await next_frame();                // one frame
```

## Doing things at the same time

`choreograph` runs several animations together and finishes when the **last** one does:

```cpp
co_await choreograph(
	fade_in( stage, title, 0.5s ),
	grow_in( stage, circle, 0.8s ),
	narration.play() );
```

When the number of animations is only known at run time, collect them in a `std::vector<Coroutine>` and use
`choreograph_all`:

```cpp
std::vector<Coroutine> running;
for ( Visual *item: items )
	running.push_back( fade_in( stage, *item, 0.4s ) );
co_await choreograph_all( std::move( running ) );
```

To start things one after another with an overlap, use `stagger` (see [Animation verbs](../guides/animation.md)).

## Running something every frame

For your own animations, **`tween`** calls a function once per frame with how far along it is, 0 to 1, shaped by
an [ease](../guides/animation.md#easing):

```cpp
co_await tween( stage, 1.5s, ease_cubic_in_out, [&]( float t )
{
	circle.position = { -4.F + 8.F * t, 0.F };
} );
```

`tween` always makes at least two frames, a first (`t = 0`) and a last (`t = 1`, exactly).

To keep **one thing tied to another** for as long as you like, add an *updater*. It runs every frame, after the
scene has done its step and before the picture is drawn:

```cpp
auto tie = stage.updaters.add( [&] { label.position = dot.position + Point{ 0.F, 0.4F }; } );
co_await move_along( stage, dot, path, 2s );     // the label goes with the dot
```

The returned handle ends the updater when it is destroyed; call `tie.release()` to let it run to the end of the
scene instead. Updaters are cleared when a scene starts. If you remove a visual an updater uses, end the updater
first.

## Lifetimes

An animation refers to the visual it works on, by reference. The visual **must stay on the stage until the
animation is over**. In particular, do not `clear()` the stage or morph a visual away while an animation that
uses it is still running in a `choreograph`.

## Frame rate and length

Nothing in a scene depends on the frame rate: times are in seconds, rounded to the nearest whole frame. A time
that is not a whole number of frames (half a second at 25 fps) is a frame more or less. The render's length is
however long the scene takes, including the narration it waits for.
