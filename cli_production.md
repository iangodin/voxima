# CLI Production Architecture Manifesto

This document outlines the refactoring plan for `Producer.cpp` to move from a list-based output system to a data-driven production configuration.

## Goals
1. **Support Multi-Language Production**: Generate audio and subtitle tracks for multiple languages.
2. **Flexible Muxing**: Support "Master Language" mode (one video, multiple tracks) vs. "Per-Language" mode (one video file per language).
3. **Codec Control**: Allow switching between H.264 and H.265 without manual file manipulation.
4. **Improved DX**: Simplify CLI usage with an `--output-dir` and auto-naming.

---

## Phase 1: Configuration & CLI Parsing
- **New Structures**:
    - `struct LanguageTrack { std::string lang; std::string audio_path; std::string sub_path; }`
    - `struct ProductionJob { std::string output_base; std::string master_lang; std::vector<std::string> languages; bool h265; std::string output_dir; }`
- **CLI Options**:
    - `--lang <list>`: Comma-separated or repeatable language tags.
    - `--master-lang <lang>`: Specify which language is the "primary" stream.
    - `--vcodec <h264|h265>`: Video codec preference.
    - `--output-dir <path>`: Base directory for all artifacts.
    - `--include-subtitles`: Flag to trigger subtitle generation.

## Phase 2: Pipeline Refactoring
- **Producer Loop**: 
    - Separate the *rendering phase* (SVG/Bitmap generation) from the *muxing phase*.
    - If `--master-lang` is set, run the render loop once for the visuals, but loop over `--lang` to generate audio/subtitles.
    - If `--master-lang` is NOT set, run the full render loop for every language in the `--lang` list.
- **Temporary File Handling**: Use a dedicated `render/temp/` folder for raw assets during the render loop. Remove manual renaming; rely on explicit output paths passed to `FfmpegRender` and `AudioMixer`.

## Phase 3: Subtitle Infrastructure
- **Subtitle Manager**: Add a `SubtitleManager` (similar to `Narration`) in `src/tts/` (or `src/utils/`). 
- **Data Collection**: During the render loop, collect cue data (text, start, duration) from the `SubtitleManager`.
- **Export**: Write this data to a standard format (e.g., `.srt` or `.vtt`) before the final muxing step.

## Phase 4: FFmpeg Muxer Enhancement
- **Generalized Muxing**: Update `FfmpegRender::mux` to accept a list of inputs:
    - Input 0: Primary Video.
    - Inputs 1..N: Audio/Subtitle tracks.
- **FFmpeg Map Logic**: Automatically construct the `-map` and `-c` (codec) arguments based on the input list to ensure proper stream labeling (e.g., `-metadata:s:a:0 language=en`).

---

## Implementation Notes
- **FfmpegRender**: Should no longer "close" itself. The `Producer` should control the lifecycle to allow for multiple rendering passes.
- **AudioMixer**: Needs to ensure that different language renders don't conflict. The `output_dir` should be used to scope file paths for each language iteration.
