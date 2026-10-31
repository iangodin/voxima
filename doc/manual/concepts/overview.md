# Overview

A Voxima video is made of three things, kept apart on purpose:

| | What it is | Where it lives |
|---|---|---|
| **Scenes** | C++ coroutines that decide what is drawn and when | A **plugin** you build (`.so` / `.dll`) |
| **Script** | The words: what is shown and what is narrated, in each language | `<lang>.huml` files |
| **Theme** | The fonts and colors | A built-in theme, or a `.huml` theme file |

The `voxima` program has no scenes of its own. You give it a plugin (`--plugin`), a directory of scripts
(`--script-dir`), the languages (`--lang`) and a theme, and it renders the scene called `movie` (or the ones you name
with `--scene`) to a video with narration and subtitles, or plays it in a window (`--preview`).

```
                          voxima --plugin hello --script-dir scripts --lang en --lang fr
                                         |
   hello.so (scenes) ───────────────────►|◄─────────────── scripts/en.huml, fr.huml (words)
                                         |◄─────────────── theme (fonts, colors)
                                         ▼
                      final.mkv   (video + one audio and one subtitle track per language)
```

Keeping the words out of the C++ means one scene serves every language, and a writer can change the narration
without recompiling anything.

## The stage

A scene draws on a `Stage`. Its parts:

- `stage.visuals`: the group of everything that is drawn. You `add` visuals to it.
- `stage.theme`: the fonts and the palette (`stage.theme->palette().blue`).
- `stage.camera`: what part of the stage is seen (and the picture's size and frame rate).
- `stage.audio`: the sound: narration, music and effects.
- `stage.updaters`: small functions run every frame, to keep one thing tied to another.

**Coordinates.** The stage is measured in *stage units*, with **y pointing up** and the origin at the center of
the picture. At the default zoom the picture is **16 units wide**; at 16:9 that is 9 units tall, so the visible
area runs about from -8 to 8 across and -4.5 to 4.5 up. Resolution does not matter to a scene: `--width 1920`
makes the same picture sharper, not bigger. Everything is vector graphics until the last moment.

**Time.** Durations are written with units (`0.5s`, `250ms`) and turned into whole frames at the render's frame
rate. A bare number does not compile, so a scene never has to guess between seconds and frames.

**Color.** A `Color` is red, green, blue and alpha, each 0 to 1. Pure white means "the theme's text color", so a
scene written for a dark background stays readable on a light one. For anything that should suit any palette,
use the palette's accents (`red orange yellow green cyan blue purple pink`, plus `muted`) instead of fixed
colors. See [Themes](../guides/themes.md).

## Visuals

Everything drawn is a `Visual`: a position, a rotation, a scale, a color, an outline. Shapes (`Circle`,
`Rectangle`, `Line`), laid-out text, formulas, charts and diagrams are all visuals, and most are a `Group`
holding other visuals (a piece of text is a group of lines of words of glyphs). Add one to the stage with
`stage.visuals.add( ... )`, which gives you back a reference to keep for animating it.

Two rules to remember:

- A visual **stays alive as long as it is in a group**. The reference `add` returns is valid until you remove or
  clear the visual.
- A scene **starts on an empty stage**, so it looks the same on its own as inside a movie.

## Where to read next

- [Scenes and movies](scenes.md): registering scenes, playing several, choosing what to render.
- [Animation and coroutines](coroutines.md): how a scene runs, and how to run things together.
- [The script](script.md): the words and languages.
