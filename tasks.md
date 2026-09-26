
# Sonimation Task List

## Text Rendering
  - [x] Create StyledText class to support styling text
  - [x] Pass current text language all the way to Harfbuzz to help shape/layout gylphs
  - [x] Integrate handling for BiDi text rendering
  - [x] Group each word's glyphs into a tagged group (word0, word1, ...) sharing a word splitter with narration
  - [x] Text line spacing (scales with font size) and left/center/right alignment
  - [x] Highlight missing glyphs (0xFFFD and characters no font has) as magenta boxes with their code, listed at the end of the render

## Visuals
  - [x] Axes: ticks at nice steps, numbers (with a halo), arrow tips, axis names, a grid (make_axes)
  - [x] Graphs of functions: smooth, clipped to the plot, broken at asymptotes, drawn on along their length (plot, draw)
  - [x] Axes along the edges or crossing in the middle (AxesOptions::placement), the grid as one path (no bright spots at crossings)
  - [x] Curve colors from the palette (the accents in turn)
  - [ ] Points and scatter plots, bar charts, a legend
  - [ ] Parametric and polar curves; a point moving along a curve with its tangent; shaded areas (Riemann sums)
  - [ ] Axes drawn on (growing, ticks and numbers appearing), and graphs morphing into other graphs
  - [ ] Import external SVG files
  - [x] Add support for images / photos

## Math Expressions
  - [x] Build math::Expr tree (no parser, expressions built directly in C++)
  - [x] Text atoms (italic/upright/bold) using the math alphabets
  - [x] Fractions and binomials
  - [x] Radicals and roots
  - [x] Superscripts / subscripts
  - [x] Large operators (sum, product, integral) with limits
  - [x] Stretchy delimiters / fenced expressions (parens, brackets, braces, bars)
  - [x] Accents, overline, underline
  - [x] Rows with TeX-style atom spacing (ord/op/bin/rel/open/close/punct/inner)
  - [x] Matrices and arrays (matrix, pmatrix, bmatrix, vmatrix, cases)
  - [x] Tagging parts for later lookup/animation
  - [x] Explicit style override (\textstyle / \displaystyle equivalent)
  - [x] Named operators with limits (operatorname, lim, max, min, sin, cos, log, ...)
  - [x] Expose remaining math alphabets on Expr (script, fraktur, double-struck/blackboard, sans, mono)
  - [x] Dots (ldots, cdots, vdots, ddots)
  - [x] Over/underbrace
  - [x] Stacked relations (stackrel)
  - [x] Phantom / invisible spacing placeholder

### Math Expressions: not there yet
A review of `math/Expr.h` against what a page of teaching math needs. What is listed above is all done, and complete for the TeX constructs it covers; these are the gaps:
  - [x] Aligned multi-line equations: `aligned( rows, { .numbered = true } )` (pairs of cells right/left, equation numbers or tags)
  - [x] Text inside math: `words( "for all" )` (upright, spaces kept)
  - [x] Marking a part: `cancel`, `boxed`, `colored`
  - [x] Primes after a symbol: `primed( f, 2 )`
  - [ ] Stretchy wide accents (`widehat`, `widetilde`) and arrows over a base (`overrightarrow`) that span their content
  - [ ] Stacked limits on several lines (`substack`), and continued fractions
  - [ ] Delimiters of a chosen size (`\big`, `\Big` ...), not only the ones that stretch to fit
  - [x] Symbols by name (`math/Symbols.h`: `sym::alpha`, `sym::leq` ...)
  - [ ] Shorter ways to build rows (operators for the common `a + b = c`)

## Composition and Diagrams
The biggest gap: everything is placed by hand-typed coordinates. The building blocks of most teaching content:
  - [x] Arrange: rows, columns and grids of visuals with a gap, aligned (start, center, end, baselines); next_to( a, b, side, gap )
  - [x] Anchors: the edges, corners and center of any visual (point_at, move_to), so things are placed relative to each other, not to the stage
  - [x] Panels: a rounded box that fits its contents (padding, radius, outline), in the text color's tint so it suits any palette
  - [x] Callouts: a panel with a pointer to what it is about (`make_callout`); panels can have a shadow (`PanelOptions::shadow`)
  - [x] Lists (bullets, numbers; several lines an item) and tables (columns aligned left, center or right; header; rules), on a fixed line step so rows are evenly spaced
  - [x] Distribute (equal spaces between things of different sizes), and align a set of things to one edge
  - [x] Text wrapped to a width: `wrap_lines`, `make_paragraph` ; lists wrap with `ListOptions::max_width` ; tables wrap with `TableOptions::max_width`, take any visuals as cells, and can tint rows (`striped`, `header_fill`)
  - [x] Connectors: arrows and lines that join two visuals and follow them when they move ; braces with labels: `make_brace`, `make_labeled_brace_beside`
  - [x] Diagrams: nodes and edges with a layered layout (`Diagram`; flowcharts, trees). Number lines: `make_number_line` (marks, intervals). Geometry: `make_polygon`, `make_angle`.
  - [x] Safe areas: `safe_area`, `keep_inside`, `fit_inside` (open: a visible grid to lay out on)

## Animation Vocabulary
The stock verbs, so a scene says what it means. We have tween, morph, choreograph and the word effects; missing:
  - [x] Updaters: something run every frame that keeps one thing tied to another (a label that follows a moving point, a connector between two moving boxes)
  - [x] Draw-on (Create) and Write for any visual: strokes drawn along their length, text written; `draw_on` does both; open: the reverse (Uncreate)
  - [x] Fade in and out, grow and shrink from a point, slide in from an edge, for any visual
  - [x] Indicate (`indicate`), flash and circumscribe: draw the eye to something (open: highlighting a part of a visual)
  - [x] Move along a path (`move_along`, optionally turning to face the way it goes), and turn about a point (`turn_about`)
  - [x] Transitions between scenes: fade out, fade in (`PlayOptions`; the movie uses them). Open: cross-fade, wipe, slide
  - [x] Staggered groups (`stagger`, `children_of`). Open: spring and other physics-like eases

## Math Authoring
  - Math is written as C++ expressions building the Expr tree directly (`math/Expr.h`); there is no string parser, and none is planned. Anything that makes those expressions shorter to write belongs in the gaps listed under Math Expressions instead.
  - [ ] Step-by-step transformations of an equation (one line to the next, showing which parts move), on top of the morph
  - [ ] Highlight a part of a formula in narration (by tag, as words are)

## Authoring Loop
Scenes are compiled C++, so every change is a rebuild.
  - [ ] Hot reload: change the script (or a theme file) and the preview updates
  - [ ] Scenes as data: which could be written without C++ (as themes now are), and how far that goes
  - [ ] A project template (a new video: scripts, theme, scene, build), and tutorials
  - [ ] Visual regression: a tool to render chosen frames and compare them with a saved set (we compared by hand many times)

## Output and Integration
  - [ ] Burned-in subtitles (there are only sidecar tracks muxed in), with a style from the theme
  - [ ] Transparent background (to composite over something else), and other formats (GIF, ProRes)
  - [ ] Chapters in the output, from the scenes
  - [ ] Pronunciation control for the narration (a dictionary, a respelling in the script)
  - [ ] Loudness normalisation of the mix (LUFS), and the rest of audio.md's chain (gate, compressor, EQ)

## Not Planned
  - 3D: it would be another renderer.
  - A LaTeX (or any string) math parser: formulas are built in C++ from `Expr`, by design.

## Source Code
  - [x] Syntax highlighting of C and C++ with tree-sitter (code::highlight)
  - [x] make_code: highlighted code in the theme's monospaced font, a code palette per theme
  - [x] Typing: a character at a time, with a person's rhythm (Typewriter)
  - [x] Highlight one line, a range, or several places (LineHighlight): the rest dimmed, a band behind them, sliding from one to the next
  - [ ] Highlight parts of a line (a word, a range of columns)
  - [ ] Morph one version of some code into the next (an edit), lines moving to make room
  - [ ] A panel behind the code, and line numbers
  - [ ] Tags for parts and lines that the morph ignores (as it does word tags)
  - [ ] More languages (Python, JavaScript ...: grammars vendored, not in Conan)

## Geometry Processing
  - [x] Flatten any visual to cubic outlines (Outline: circles, rectangles, paths, glyphs, groups with transforms)
  - [x] Morph between shapes and glyphs (feature-aware contour alignment, holes open/close, color/stroke blend)
  - [x] co_await morph( stage, from, to, duration, ease ) and a scrubbable Morph object
  - [x] Morph groups: match parts by tag / identical glyph / proximity, fade the rest (formulas, x^2 -> x^3)
  - [x] Text and narration: word-aware text morphs, and morphs driven by the narration (track_words)
  - [x] Morph lines and arrow tips (open stroke contours; a line can inflate into a disc)
  - [x] A shape only turned or moved is carried round whole (an arrow turning keeps its head)
  - [x] Image cross-fade (same picture moves, different pictures fade) - needs opacity in the SVG writer
  - [x] Stagger (parts start one after another) and per-part ease
  - [x] Cap on anchors tried for very dense outlines
  - [ ] A stroke turning into a fill as its filled outline (a line looks half a pixel bolder while it starts to inflate)

## Audio System
  - [x] Need a AudioMixer that mixes everything and generates the final audio to a WAV file
  - [x] Need a DubbingStage that handles several narrators/mixers for multiple audio tracks

## Audio Processing
  - [x] Implement basic audio buffer
  - [x] Implement audio resampling
  - [x] Add WAV file saving
  - [x] Add FLAC file saving
  - [x] Implement silence generator
  - [x] Implement simple volume effect
  - [x] Implement simple multi channel mixer

## Music
  - [x] Add music file format reading (WAV, FLAC, Ogg, MP3 through libsndfile; made stereo and resampled)
  - [x] Need a music player to support playing/looping (stage.audio.play: volume in dB, loop, fade in/out, start offset)
  - [x] Sound effects: any number of sounds at once, from files or made in the program
  - [ ] Ducking: music quieter while the narrator speaks (per language, keyed by that language's voice)
  - [ ] Loop points inside a track (loop a section, not the whole file)
  - [ ] A proper limiter at the end of the chain (a hard clip for now)

## Text-To-Speech
  - [x] Create python Edge-TTS bridge server
  - [x] Add C++ client to generate voice audio
  - [x] Decode Edge-TTS audio into float/PCM buffer
  - [x] Need "Speaker" that can generate audio with a language and voice parameters

## Narration Sync
  - [x] Expose per-word narration timing to animation (forced alignment, proportional fallback)
  - [x] co_await API: until_word / until_word_end / until_time / on_words
  - [x] Karaoke word highlighting demo
  - [x] Try the demo with --lang fr, Polish and no aligner model
  - [x] Per-word animation effects as coroutines: reveal, color, pop, underline (animate_words, word_effects::)

  - [x] Markers: cue points ({name}) in the script text, the same in every language, with a time in each language's narration (marker_time / until_marker / track_markers)
  - [x] One container per language, each timed to its own narration (--separate)
  - [x] Single video with several audio tracks: one timeline per marker segment, shorter languages padded with silence
  - [x] Accelerate other languages up to --max-speedup (ffmpeg atempo, shortening pauses first)
  - [ ] Effects over a span between two markers (progress across the glyphs), and subtitles that break at markers

##  Subtitles
  - [x] Generate SRT subtitles files
  - [x] Generate WebVTT subtitles files
  - [x] Mux subtitles into final stream

## Language Support
  - [x] Handle BCP-47 / ISO 639 conversion

## Themes and Colors
  - [x] Palettes apart from fonts: roles (background, text, muted, eight accents), dark and light, chosen with --palette
  - [x] Light backgrounds: pure white is the theme's text color, so a scene reads on either; code, plots and highlights from the palette
  - [x] Themes as files (themes/*.huml): fonts as families in stacks, a palette, and inherit; the five built-in themes converted, checked against a record of every font they load
  - [x] --theme takes a file, --theme-dir, and theme.huml next to the scripts is used by default
  - [ ] Palettes from a file (a palette section in a theme file)
  - [x] The missing-glyph marker cannot be lost to a background or neighbour of its own color (a keyline in the text color), and the preview reports missing glyphs too
  - [x] The demo scenes use the palette's accents, not fixed colors, so they read on light and dark (a scene of your own: theme->palette().blue)
  - [x] Math on a light background: every part of an Expr follows the text color (fraction bars, radicals, brackets, big operators, accents, braces), checked on the formulas scene

## Camera
  - [x] Pan and zoom: Camera::center and zoom, and camera_to / camera_frame (zoom by ratio, the pan following so what is zoomed toward holds still on the screen)
  - [ ] Rotate the camera
  - [ ] Keep line widths on screen when zoomed (hairlines), as an option
  - [ ] Follow a moving visual

## Rendering
  - [ ] Add support for motion blur
  - [x] Add registry of "Scenes" (SONIMATION_SCENE, play/sequence for movies; see doc/scenes.md)
  - [x] Add command line option to select a scene to render (--scene, repeatable; --list-scenes)

## Animation Timing
  - [x] Times in seconds, not frames (wait_for( stage, 0.5s ), morph( ..., 1.5s, ... )), so --fps does not change how long things take
  - [x] Eases as plain functions of t (ease_cubic_in_out( t ) ...), and tween( stage, duration, ease, callback )
  - [ ] Frame rates that do not divide the audio rate, and fractional ones (29.97, 23.976)
  - [ ] Keep chained waits from drifting by a frame or two (rounding each duration to whole frames)

## Final Muxing
  - [x] Need to use ffmpeg to mix the audio and video (simple proof of concept)
  - [x] Need to use ffmpeg to mix the multiple audio tracks
  - [x] Need to use ffmpeg to mix the multiple subtitle tracks

## Preview
  - [x] Use SDL to present video/audio as it renders (--preview, first --lang only)
  - [x] Producer thread renders ahead into an in-memory frame store (SVG and 16 bit audio per frame)
  - [x] Play forward/backward (audio reversed too), pause, step frames with a short sound, skip 1 second, start/end
  - [x] Scene start/end recorded; previous/next scene ([ and ]) and repeat the current scene (r)
  - [x] Frame rate from the display's refresh rate when --fps is not given
  - [x] Prefer X11 on Linux (the Conan SDL has no libdecor, so Wayland windows have no title bar on GNOME)
  - [ ] Timeline bar: click to seek, scenes marked, produced part filled in
  - [ ] On-screen overlay (scene, time, frame, waiting) instead of only the window title
  - [ ] Switch language while previewing
  - [ ] Smooth the audio clock if frames are skipped on real devices
  - [ ] Suppress audio.cpp's OpenMP false positives in make tsan

