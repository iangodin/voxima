
# Sonimation Task List

## Text Rendering
  - [ ] Create StyledText class to support styling text
  - [ ] Pass current text language all the way to Harfbuzz to help shape/layout gylphs
  - [ ] Modify MFL library to return a tree of glyphs instead of linear sequence
  - [ ] Integrate handling for BiDi text rendering
  - [ ] Add "highlighting" of missing glyphs (0xFFFD)

## Visuals
  - [ ] Render plots and charts
  - [ ] Import external SVG files
  - [ ] Add support for images / photos

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
  - [ ] Implement silence generator
  - [ ] Implement simple volume effect
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
  - [ ] Generate SRT subtitles files
  - [ ] Generate WebVTT subtitles files
  - [ ] Mux subtitles into final stream

## Language Support
  - [ ] Handle BCP-47 / ISO 639 conversion

## Rendering
  - [ ] Add support for motion blur
  - [ ] Add registry of "Scenes"
  - [ ] Add command line option to select a scene to render

## Final Muxing
  - [x] Need to use ffmpeg to mix the audio and video (simple proof of concept)
  - [x] Need to use ffmpeg to mix the multiple audio tracks
  - [ ] Need to use ffmpeg to mix the multiple subtitle tracks

## Preview
  - [ ] Use SDL to present video/audio as it renders

