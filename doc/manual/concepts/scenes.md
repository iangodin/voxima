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
REGISTER_SCENE( "intro", "The title and the first formula", show_intro );
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
REGISTER_SCENE( "movie", "The whole thing", movie );
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
voxima --plugin build/myvideo --list-scenes   # the registered scenes and what they do (see Plugins)
voxima ...                                  # renders the scene called "movie"
voxima ... --scene morph-text               # one scene
voxima ... --scene morph-shapes --scene karaoke   # several, in the order given (a movie made on the spot)
```

An unknown name is reported (with the list) before any model or font is loaded, and exits with status 1. If no
scene is called "movie" and there is no `--scene`, the only registered scene is used; with several and no
"movie", the list is shown as an error.

## Plugins: scenes outside the program

`voxima` itself has no scenes: they are in **plugins**, shared libraries (a `.dll`, or a `.so`) loaded with
`--plugin` (repeatable; the extension may be left out). A plugin is its scenes, registered as above, and one line
in one of its files (`core/Plugin.h`):

```cpp
VOXIMA_PLUGIN();
```

```
voxima --plugin build/myvideo --list-scenes
voxima --plugin build/myvideo ... --scene karaoke
```

In CMake a plugin is `voxima_add_plugin( name sources... )` (see [Your own plugin project](../guides/plugin-project.md)).

A plugin shares the program's strings, functions and coroutines, so it must be **built just as the program was**:
the same compiler and version, standard library, and Debug or Release, against the same Voxima. Each says what
it was built with, and one that does not match is refused when it is loaded, naming both, rather than crashing.

**With `--preview`, a rebuilt plugin is loaded again by itself**: the scenes start again with the new code, and
the window goes back to where it was in the scene being watched. F5 does the same, and loads the script again
too. If the new build cannot be loaded, or a scene fails, it is shown in the window's title (and on stderr), and
the next build is tried when it comes. (What is loaded is a copy, in the temporary directory, so the build can
write over the file.)

See [Installing](../getting-started/install.md) and [Your own plugin project](../guides/plugin-project.md) for
building Voxima and building a plugin against it.

Languages and narration are in [Narration](../guides/narration.md#several-languages); missing glyphs in
[Rendering](../guides/rendering.md#missing-glyphs).
