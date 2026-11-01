# Changelog

## 0.1.0-alpha.1 (2026-10-31)

The first release. The API will change between alphas.

- **Scenes as coroutines**: a scene is a C++20 coroutine (`co_await fade_in( ... )`), built into a plugin that the
  `voxima` program loads. Plugins are checked against the program's build when they load. With `--preview`, a
  rebuilt plugin is loaded again.
- **Visuals**: shapes, images, panels, callouts, lists, tables, connectors, braces, diagrams, number lines and
  geometry, placed with anchors, rows, columns, grids and safe areas.
- **Text**: shaping with HarfBuzz, right-to-left and mixed-direction text, wrapping, and missing glyphs shown and
  listed (`--strict-glyphs`).
- **Math**: formulas built in C++ (`math::Expr`) and laid out as TeX would: fractions, roots, scripts, large
  operators, delimiters, matrices, aligned equations, accents and braces.
- **Charts**: axes, graphs of functions, scatter plots, bar charts and legends.
- **Code**: syntax highlighting of C and C++ (tree-sitter), typing, and line highlights.
- **Animation**: tweens, eases, morphing between outlines, draw-on, fades, grows, slides, indicate, moving along a
  path, staggering, updaters, word effects, the camera, and fades between scenes.
- **Narration**: text to speech on your own machine (Chatterbox, through audio.cpp; CPU, or CUDA with
  `VOXIMA_TTS_CUDA=ON`), with optional word timing (Qwen3 forced aligner) for animation that follows the words.
- **Languages**: one script file for each language, one video with an audio and subtitle track for each language
  (`--max-speedup`) or one video for each language (`--separate`).
- **Sound**: background music and sound effects, mixed into every language's audio.
- **Themes**: five built-in themes (Modern Technical is the default), theme files that inherit from them, and
  palettes chosen apart from the fonts (`--palette`).
- **Output**: video through ffmpeg (mp4, mkv or webm; h264, h265, vp9 or av1), PNG and SVG frame sequences, and a
  preview window with sound.
- **Building**: `make install` gives a CMake package (`find_package( Voxima )`, `voxima_add_plugin`). Ten examples
  come as plugin projects, and the developer manual is in `doc/manual`.

### Known limitations

- The CUDA speech backend (`VOXIMA_TTS_CUDA=ON`) is not built or tested in CI, which builds and tests the CPU
  backend on Linux, Windows and macOS.
- Chinese and Japanese scripts are not spoken: audio.cpp's Chatterbox model does not list those languages.
- Subtitle tracks have been checked in mkv files; in mp4 and webm files they have not.
