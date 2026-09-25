# Scenes and movies

A **scene** is a coroutine that animates the stage: `Scene f( Stage &, Script & )` (`Scene` is just
`Coroutine`). Scenes are registered by name, so any one of them can be rendered on its own, and a **movie** is
a scene that plays other scenes.

## Registering a scene

Put one line next to the scene (`core/SceneRegistry.h`):

```cpp
Scene show_intro( Stage &stage, Script &script )
{
    // build visuals on stage.visuals, co_await animations, narration, other scenes ...
}
SONIMATION_SCENE( "intro", "The title and the first formula", show_intro );
```

The function, or any callable, only has to convert to a `Director`
(`std::function<Coroutine( Stage &, Script & )>`); a scene that needs more arguments is wrapped in a lambda.
There is no list to edit: adding a scene is writing it and adding that line.

## A movie

A movie is an ordinary scene whose body plays others:

```cpp
Scene movie( Stage &stage, Script &script )
{
    co_await sequence( stage, script, { "intro", "morphs", "outro" } );
}
SONIMATION_SCENE( "movie", "The whole thing", movie );
```

- `play( stage, script, "name" )` plays one registered scene and finishes when it does.
- `sequence( stage, script, { ... } )` plays several, one after another.
- Scenes can also run at the same time: `co_await choreograph( play( ... ), play( ... ) )`.
  (Each `play` clears the stage first, so do that only for scenes that share nothing.)

**The stage is cleared before each scene** (`PlayOptions::clear_stage`, on by default), so a scene looks the
same on its own as in a movie and never depends on what the one before left behind. Pass
`PlayOptions{ .clear_stage = false }` for a scene that carries on from the previous one. The last scene's
visuals stay on the stage.

## Choosing what to render

```
sonimation --list-scenes                       # the registered scenes and what they do
sonimation ...                                  # renders the scene called "movie"
sonimation ... --scene morph-text               # one scene
sonimation ... --scene morph-shapes --scene karaoke   # several, in the order given (a movie made on the spot)
```

An unknown name is reported (with the list) before any model or font is loaded, and exits with status 1. If no
scene is called "movie" and there is no `--scene`, the only registered scene is used; with several and no
"movie", the list is shown as an error.

A `Producer` can still be given a `Director` in code (`Producer producer( my_director )`), used when there is
no `--scene`.

## Missing glyphs

A character that cannot be drawn is never left to fail quietly. Two things count:

- a character no real font in the theme's stack has (the catch-all NotDef font at the end of the stack draws a
  box for anything, so it does not count as having it), and
- U+FFFD, the replacement character: what a script value that is missing (`as_string( "\uFFFD" )`) or a badly
  encoded string turns into.

Each is drawn as a **magenta box with the character's code in it** (`FFFD`, `10FFFD` ...), which keeps the text's
spacing and is tagged `missing-glyph`. When the render finishes, every one is listed with the text it was in:

```
WARNING: 2 missing glyphs (drawn as magenta boxes in the video):
  U+FFFD  in "A script value that is missing: ..."
  U+10FFFD  in "A character no font has: ..."
```

`--strict-glyphs` makes the render exit with an error if there were any, for automated builds. (`--scene
missing-glyphs` shows the effect.) Text that is not valid UTF-8 is still rejected outright.
