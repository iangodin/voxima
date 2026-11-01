# The examples

Voxima comes with example plugins, one folder each in `examples/` (`<prefix>/share/voxima/examples/` once
installed). Each is a small project of its own: copy a folder out and change it. They are also what the code in this
manual is taken from, and they are built with Voxima, so they keep working.

| Example | Shows | Guide |
|---|---|---|
| `hello` | One scene: a title and a circle (the only scene of its plugin, so it plays by default) | [Your first video](first-video.md) |
| `coroutines` | Waiting, doing things together, a change every frame, an updater | [Animation and coroutines](../concepts/coroutines.md) |
| `layout` | Shapes, groups, drawing order, dashes, gradients, placement, panels, lists, tables, tags | [Visuals and layout](../guides/visuals-layout.md) |
| `text-math` | Text, styled and right-to-left text, formulas, code, missing glyphs | [Text and math](../guides/text-math.md) |
| `animation` | The animation verbs, connectors that follow, free arrows, frames and arcs, dimming, the camera | [Animation verbs and the camera](../guides/animation.md) |
| `morphing` | Shapes, letters, formulas, sentences and strokes that morph | [Morphing](../guides/morphing.md) |
| `charts` | Plots, scatter plots, bar charts, diagrams, geometry | [Charts and diagrams](../guides/charts-diagrams.md) |
| `narration` | Speech in several languages; words, lines and markers followed | [Narration and languages](../guides/narration.md) |
| `sound` | A sound made in code, and music | [Music and sound effects](../guides/sound.md) |
| `tour` | A `movie` scene that plays the other examples' scenes one after another | [Scenes and movies](../concepts/scenes.md) |

## Running them

Built inside Voxima, every example goes to `build/Release/plugins`, a folder with nothing else in it, so they can
all be loaded at once:

```
build/Release/voxima --plugin-dir build/Release/plugins --list-scenes
```

lists every scene of every example. Pick one with `--scene`, and give it a script folder (each example has a
`scripts/` folder with the words it needs; the narration scenes need the [speech models](install.md#speech-models-and-voices-only-for-narration)):

```
build/Release/voxima --plugin-dir build/Release/plugins --script-dir examples/layout/scripts --lang en \
    --scene layout-basics --preview
```

Scene names are unique across the examples (`layout-basics`, `morph-shapes`), so they can be loaded together.

## The whole tour

The `tour` example has a `movie` scene that plays the scenes of the others one after another, which is what
renders when you give no `--scene`. Load everything and give it its own script folder:

```
build/Release/voxima --plugin-dir build/Release/plugins --script-dir examples/tour/scripts --lang en \
    --theme "Academic Classic" --preview
```

Music is optional: set `VOXIMA_MUSIC` to a sound file for some under it. The narrated scenes are left out, since
they need the speech models; render them on their own with `--scene` (see the `narration` example). Some themes'
fonts lack a few math symbols, which show as magenta boxes and are listed at the end of the render.
