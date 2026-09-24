
# Sonimation Task List

## Text Rendering
  - [x] Create StyledText class to support styling text
  - [x] Pass current text language all the way to Harfbuzz to help shape/layout gylphs
  - [x] Integrate handling for BiDi text rendering
  - [ ] Add "highlighting" of missing glyphs (0xFFFD)

## Visuals
  - [ ] Render plots and charts
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

## Geometry Processing
  - [ ] Add morphing between shapes

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
  - [ ] Add music file format reading
  - [ ] Need a music player to support playinglooping

## Text-To-Speech
  - [x] Create python Edge-TTS bridge server
  - [x] Add C++ client to generate voice audio
  - [x] Decode Edge-TTS audio into float/PCM buffer
  - [x] Need "Speaker" that can generate audio with a language and voice parameters

##  Subtitles
  - [x] Generate SRT subtitles files
  - [x] Generate WebVTT subtitles files
  - [x] Mux subtitles into final stream

## Language Support
  - [x] Handle BCP-47 / ISO 639 conversion

## Rendering
  - [ ] Add support for motion blur
  - [ ] Add registry of "Scenes"
  - [ ] Add command line option to select a scene to render

## Final Muxing
  - [x] Need to use ffmpeg to mix the audio and video (simple proof of concept)
  - [x] Need to use ffmpeg to mix the multiple audio tracks
  - [x] Need to use ffmpeg to mix the multiple subtitle tracks

## Preview
  - [ ] Use SDL to present video/audio as it renders

