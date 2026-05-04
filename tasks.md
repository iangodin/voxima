
# Sonimation Task List

## Text Rendering
  - [ ] Pass current text language all the way to Harfbuzz to help shape/layout gylphs
  - [ ] Modify MFL library to return a tree of glyphs instead of linear sequence
  - [ ] Integrate handling for BiDi text rendering
  - [ ] Add caching system for text
  - [ ] Add "highlighting" of missing glyphs (0xFFFD)

## Visuals
  - [ ] Render plots and charts
  - [ ] Import external SVG files
  - [ ] Add support for images / photos

## Geometry Processing
  - [ ] Add morphing between shapes

## Audio System
  - [ ] Need a "Recording Studio" that mixes everything and generates the final audio to a WAV file

## Audio Processing
  - [ ] Implement basic audio buffer
  - [ ] Add WAV file saving
  - [ ] Implement silence generator
  - [ ] Implement simple volume effect
  - [ ] Implement simple multi channel mixer

## Music
  - [ ] Add music file format reading
  - [ ] Need a music player to support playinglooping

## Text-To-Speech
  - [ ] Research how to integrate the Sherpa-ONNX library
  - [ ] Find good Sherpa models to use to generating audio
  - [ ] Need "Speaker" that can generate audio with a language and voice parameters

## Rendering
  - [ ] Add support for motion blur
  - [ ] Add registry of "Scenes"
  - [ ] Add command line option to select a scene to render

## Final Muxing
  - [ ] Need to use ffmpeg to mix the audio tracks, subtitle, and video tracks together

## Preview
  - [ ] Use SDL to present video/audio as it renders

