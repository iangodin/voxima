# Voxima developer manual

Voxima makes educational videos from C++ code: vector graphics, math and text in any language, animation
written as C++20 coroutines, text-to-speech narration, and subtitles in every language of the video.

You write the **scenes** in C++ and build them as a **plugin**; the `voxima` program you build loads it,
and renders a video (or plays it in a window while you work). The **words** are in script files, one per
language, so one set of scenes serves every language.

## Start here

1. [Installing](getting-started/install.md): build Voxima from source, then fonts, ffmpeg and the speech models.
2. [Your first video](getting-started/first-video.md): build, preview and render a small plugin.
3. [The examples](getting-started/examples.md): small plugins for each part of this manual.
4. [Overview](concepts/overview.md): how scenes, script and theme fit together.

## Concepts

- [Overview](concepts/overview.md): the stage, coordinates, time and color.
- [Animation and coroutines](concepts/coroutines.md): how a scene runs, and how to do things together.
- [Scenes and movies](concepts/scenes.md): registering scenes, and playing several in order.
- [The script](concepts/script.md): the words, languages, markers and spoken text.

## Guides

- [Visuals and layout](guides/visuals-layout.md): shapes, groups, drawing order, dashes, gradients, placing, panels, lists and tables.
- [Text and math](guides/text-math.md): styled text, formulas, and code listings.
- [Animation verbs and the camera](guides/animation.md): fade, slide, draw on, easing, and zooming.
- [Morphing](guides/morphing.md): turning one thing into another.
- [Charts and diagrams](guides/charts-diagrams.md): axes, plots, bars, diagrams, arrows and geometry.
- [Narration, subtitles and languages](guides/narration.md): speech, syncing to it, and several languages.
- [Music and sound effects](guides/sound.md)
- [Themes](guides/themes.md): fonts and colors, and your own theme files.
- [Rendering and previewing](guides/rendering.md): output, chapters, part of a movie, live preview, missing glyphs.
- [Your own plugin project](guides/plugin-project.md): CMake, and building to match the program.

## Reference

- [Command line](reference/command-line.md)
- [Script keys](reference/script-keys.md)
- [HUML format](reference/huml.md)

The manual is for the version of the source it came with.
