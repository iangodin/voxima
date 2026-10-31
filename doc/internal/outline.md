# Voxima Tutorial Video — Outline

A tutorial series for Voxima, made with Voxima. Each chapter below is a candidate video/section; each
scene within it is a candidate rendered scene. Chapters build on each other in order.

## Chapter 1 — Getting Started
1. **What Voxima is** — vector graphics + coroutine animation + multi-lingual TTS/subtitles, in one
   pipeline
2. **Building the project** — `make config`, `make`, `make test`; project layout (`src/`, `themes/`,
   `examples/`)
3. **Anatomy of a minimal movie** — `Stage`, `Scene`/`Screenplay`, `Director`, `Producer`, rendering to
   a file vs. `--preview`

## Chapter 2 — Core Animation Model
1. **Coroutines as the animation language** — `co_await`, `wait_for`, `tween`, `choreograph`
2. **Eases** — cubic, elastic, back, linear — when to use which
3. **The animation vocabulary** — fade/grow/slide, `draw_on`/write, `indicate`/flash/circumscribe,
   `move_along`/`turn_about`, `stagger`
4. **Camera** — pan/zoom with `camera_to`/`camera_frame`

## Chapter 3 — Visuals and Composition
1. **Basic shapes** — Rectangle, RoundRectangle, Circle/Ellipse, Line, Path, Image
2. **Groups, anchors, and `next_to`** — building layouts without hand-typed coordinates
3. **Panels, callouts, and connectors** — arrows/lines that follow moving visuals
4. **Lists and tables**
5. **Diagrams** — nodes, edges, flowcharts/trees

## Chapter 4 — Text and Typography
1. **Styled text, fonts, and multi-language layout** — BiDi, shaping
2. **Text effects** — typing, word-by-word reveal, per-word animation tied to narration

## Chapter 5 — Math
1. **Building expressions in C++** — `math::Expr` (no parser, by design)
2. **Fractions, radicals, scripts, big operators, matrices**
3. **Symbols/operators shorthand** — `math/Symbols.h`, `math/Operators.h` — and step-by-step-looking
   equation morphs

## Chapter 6 — Plots and Data
1. **Axes and graphs of functions**
2. **Scatter plots, bar charts, legends**

## Chapter 7 — Source Code
1. **Syntax-highlighted code blocks** — typing effect, line highlighting

## Chapter 8 — Morphing and Shape Magic
1. **Morphing between shapes/glyphs** — morph groups, tagging parts for morph targeting

## Chapter 9 — Narration and Audio
1. **TTS narration** — `Narrator`, per-word timing, `until_word`/`on_words`
2. **Markers and multi-language sync** — separate tracks, auto speedup
3. **Music, sound effects, mixing** — `AudioMixer`, `DubbingStage`

## Chapter 10 — Subtitles, Themes, Output
1. **Generating subtitles** — SRT/WebVTT and muxing
2. **Themes** — palettes, fonts-as-stacks, light/dark, `.huml` theme files
3. **Final render and ffmpeg muxing** — CLI options (`--scene`, `--lang`, `--theme`, `--palette`)

## Chapter 11 — Putting It Together
1. **A worked example end-to-end** — script → scene → narration → render

## Notes
- Math and Plots (Chapters 5–6) are candidates for splitting into a separate "advanced" video if the
  main one gets too long.
- Each scene here maps naturally to one rendered scene, since the tutorial is itself a Voxima movie.
