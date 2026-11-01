# Changelog

## 0.2.0-alpha.1 (unreleased)

### Added

- **Dashed and dotted strokes**: `Visual::stroke_dash`, on any stroke, with `StrokeDash::dashed` and `dotted`. Tween its offset for
  dashes that march. `ConnectorOptions::dash` and `GraphOptions::dash`. `draw_on` keeps them and a morph blends them.
- **Gradient fills**: `Visual::fill_gradient`, linear or radial, on any filled shape. `fill_each_with_gradient` gives each glyph of
  text its own. A morph blends them, and a linear and a radial one pass through their average color.
- **Drawing order**: `Group::insert`, `to_front`, `to_back`, `raise` and `lower`. `MorphOptions::parent` lets what is in any group
  morph.
- **Opacity**: `Visual::opacity` and `fade_to`, to dim a visual and bring it back. A group fades as a whole, so what overlaps in it
  does not show through.
- **Arrows**: `make_arrow`, between points or along a path, drawn on with `draw`.
- **Helpers**: `make_surround`, `make_arc`, `grow_from_edge`, `ease_there_and_back` and `ease_there_and_back_with_pause`.
- **Palettes in theme files**: `palette::` holds the colors, all of them or only those that differ from a palette it inherits.
- **Part of a movie**: `--start-at` and `--stop-at`, by scene or by frame number, in renders and in the preview. What comes before
  runs with nothing drawn, so everything is as it would be.
- **Chapters**: a video of more than one scene gets them, in mp4, mkv and webm: the movie's top-level scenes, or those marked with
  `PlayOptions::chapter`. Titles can come from a script's `chapters` section.
- **Pronunciation**: a script's `pronounce` section says how whole words are said, for each language.
- **Speech**: `--tts-cache-dir` and `$VOXIMA_TTS_CACHE` put the cache anywhere, and renders can share it. Each line made is reported,
  with a total at the end. `--tts-only` makes the speech and exits.
- **Metal**: speech can use the GPU on Apple (`--tts-backend metal`). `VOXIMA_TTS_METAL=OFF` leaves it out.
- **Models and voices are found**: in `./models` and `./voices`, then `$VOXIMA_MODELS` and `$VOXIMA_VOICES`, then
  `<prefix>/share/voxima`. `make install` installs the voices, and an installed `download_models.py` saves next to them.
- **Download scripts**: `download_fonts.py` and `download_models.py` have `--help` and `--list`, and refuse what they do not know.

### Changed

- **Plugins build from the install alone**: third-party libraries are out of Voxima's public headers, and the Conan toolchain is no
  longer needed. A plugin that used FreeType, lunasvg or libsndfile through Voxima's headers has to include and link them itself.
- **What goes wrong in a scene is reported**: an exception goes to what waits for the scene, and from the outermost to an error message
  and a status of 1. It used to abort the program.
- `--tts-backend best` uses Metal on Apple, where it used to be the CPU.
- A `Path` with a stroke and no fill stays open when it is flattened, for a morph or `draw_on`. It used to be closed, with a line
  that was not drawn.
- `Producer::produce` returns false for a bad option, and true for `--help` and `--version`, so the program's exit status is right.
- Audio and subtitle tracks are labeled with three-letter language codes.

### Fixed

- The installed program could not start on macOS (its rpath was ELF's `$ORIGIN`), or on Linux (it needed Conan's copy of
  `libpulsecommon`, and took libsndfile's functions from it). SDL opens the sound libraries itself, from the system.
- mp4 audio and subtitle tracks had no language.
- `download_fonts.py --help` made a folder called `--help` and downloaded the fonts into it, and `download_models.py --help` started
  the download.
- Every installed header compiles by itself, and with `-Wall -Wextra -Werror`.

### Building

- CI runs the installed program, builds the examples against the install, and builds on `macos-26`.
- The build uses `sccache` where it finds one.

### Known limitations

- Speech on Metal is built in CI but has not been run. `--tts-backend cpu` is the way back on a Mac.
- The CUDA speech backend (`VOXIMA_TTS_CUDA=ON`) is not built or tested in CI, which builds and tests the CPU
  backend on Linux and Windows.
- Chinese and Japanese scripts are not spoken: audio.cpp's Chatterbox model does not list those languages.
- Gradients are 8 bits a channel with no dithering, so long dark ramps show bands.

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
