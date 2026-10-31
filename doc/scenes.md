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
voxima --plugin build/Release/eases --list-scenes   # the registered scenes and what they do (see Plugins)
voxima ...                                  # renders the scene called "movie"
voxima ... --scene morph-text               # one scene
voxima ... --scene morph-shapes --scene karaoke   # several, in the order given (a movie made on the spot)
```

An unknown name is reported (with the list) before any model or font is loaded, and exits with status 1. If no
scene is called "movie" and there is no `--scene`, the only registered scene is used; with several and no
"movie", the list is shown as an error.

A `Producer` can still be given a `Director` in code (`Producer producer( my_director )`), used when there is
no `--scene`.

## Plugins: scenes outside the program

`voxima` itself has no scenes: they are in **plugins**, shared libraries (a `.dll`, or a `.so`) loaded with
`--plugin` (repeatable; the extension may be left out). A plugin is its scenes, registered as above, and one line
in one of its files (`core/Plugin.h`):

```cpp
VOXIMA_PLUGIN();
```

```
voxima --plugin build/Release/eases --list-scenes
voxima --plugin build/Release/eases ... --scene karaoke
```

The demo is one (`demos/eases`, built as `build/Release/eases`); `render.sh` and `render.ps1` load it. In CMake a
plugin is `voxima_add_plugin( name sources... )` (`cmake/VoximaPlugin.cmake`).

A plugin shares the program's strings, functions and coroutines, so it must be **built just as the program was**:
the same compiler and version, standard library, and Debug or Release, against the same Voxima. Each says what
it was built with, and one that does not match is refused when it is loaded, naming both, rather than crashing.

**With `--preview`, a rebuilt plugin is loaded again by itself**: the scenes start again with the new code, and
the window goes back to where it was in the scene being watched. F5 does the same, and loads the script again
too. If the new build cannot be loaded, or a scene fails, it is shown in the window's title (and on stderr), and
the next build is tried when it comes. (What is loaded is a copy, in the temporary directory, so the build can
write over the file.)

## Installing, and plugins of your own

```
make install PREFIX=<dir>
```

installs `voxima`, its core library and headers, and the CMake package a plugin project finds with
`find_package( Voxima )` (`conan create .` makes a Conan package of the same). `make package` makes a
package of it to hand out, in `build/packages`: a `.zip` on Windows (with the MSVC runtime in it), a `.tar.gz`
and a `.deb` elsewhere (`-DCPACK_GENERATOR=...` for others). Its name says the compiler it was built with, which a
plugin built against it must use too. It holds these documents and `examples/` (in `share`), and the licenses: Voxima's (MIT), and
those of everything it is built with (`share/doc/Voxima/licenses`, see `THIRD_PARTY_NOTICES.md`). `examples/` is such a
project, with how to build and run it. The fonts are not installed (they are big): `download_fonts.py`
(installed in `share/voxima`) fetches them to a directory given, and without `--font-dir` voxima looks in
`assets/fonts`, then `$VOXIMA_FONTS`, then the installed `share/voxima/fonts`. The TTS models are not
installed either: give them with `--tts-model` and the like (see `tts.md`). Scenes without narration need none.

## Languages: one video for all, or one per language

```
voxima ... --lang en --lang fr              # one container with an audio and a subtitle track for each language
voxima ... --lang en --lang fr --separate   # one container per language, in <render-dir>/en/ and <render-dir>/fr/
```

- **Together** (the default): one video, timed to the first language (the master), with an audio and a
  subtitle track for each language. The languages are **kept in step**: the narration is divided at its markers and
  at the start of each line, and each segment is given the length the slowest language takes, so a marker falls
  at the same time in every language's audio (within 40 ms, under a frame). The faster languages get silence at the
  end of a segment; nobody is speeded up or cut off. A language whose markers or lines differ from the master's
  cannot be kept in step: it is warned about and left as it is.
  **`--max-speedup 1.15`** lets the other languages be speeded up by up to 15% (default 1.0: not at all) so that
  the shared timeline is shorter and they need less silence. The master language is never speeded up. A segment
  that has to be made shorter loses its pauses first (each is shortened, down to 80 ms, before anything else
  changes), and only if that is not enough is the speech itself speeded up by what is left, with ffmpeg's
  `atempo`, which keeps the pitch (so ffmpeg must be installed for this; without `--max-speedup` it is not used).
- **`--separate`:** each language is a complete render of its own: its narration and its subtitles, the script's
  text for that language on screen, and animation timed to *its* narration, through the markers in the script
  (`{name}`, see `tts.md`). Each folder holds `final.<container>` and the pieces it was made from. The models are
  loaded once for all of them. It is up to the scene to adapt to the narration: use `until_marker`,
  `track_markers` and the like rather than fixed frame counts, so that it follows whatever each language takes.

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
