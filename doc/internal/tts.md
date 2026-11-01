# Narration (TTS and alignment)

How the speech is made, and why it is made this way. How to use it in a scene is in the manual
(`doc/manual/guides/narration.md`).

## Models

- **Speech: Chatterbox Multilingual** (Resemble AI, 0.5B, MIT), as a Q8_0 GGUF (`--tts-model`, default
  `models/Chatterbox-GGUF/chatterbox-q8_0.gguf`). It clones a voice from a short reference clip, so it covers many
  languages with no fixed voices.
- **Word timing: Qwen3 Forced Aligner** (0.6B, Apache-2.0), as a GGUF (`--aligner-model`, default
  `models/Qwen3-ForcedAligner-0.6B-GGUF/qwen3-forced-aligner-0.6b-q8_0.gguf`). Chatterbox gives no word times, so a
  second pass lines the known text up with the made audio. It is optional, and loaded only when first used.

Both come from the `audio-cpp/audio.cpp-gguf` Hugging Face repository (`download_models.py`), into `models/`, which
is not committed. That repository bundles many models; each keeps its original license, and the GGUF conversion adds
no terms of its own.

Rejected, and why:

| Option | Why not |
|---|---|
| edge-tts (what was used before) | Microsoft's cloud service through an unofficial API: needs the network, can change without notice |
| Piper | GPL-3.0, fixed voices |
| Kokoro-82M | No Polish |
| Coqui XTTS v2 | Non-commercial license, company gone |
| Supertonic 3 | Unclear license, project looks abandoned |
| Meta MMS-300M (aligner) | CC-BY-NC-4.0; audio.cpp may not redistribute its GGUF |

## Runtime: audio.cpp

`extern/audio.cpp` is [audio.cpp](https://github.com/0xShug0/audio.cpp) (ggml, Apache-2.0), vendored with
`git subtree add --squash` (not a submodule: everything it needs is already in its tree). It is built in every
configuration, with only the model families used (`AUDIOCPP_MODEL_SET=custom`,
`AUDIOCPP_MODELS=chatterbox,qwen3_forced_aligner`; the aligner pulls in `qwen3_asr`'s shared front end). It is added
before Voxima's `-Werror` flags so they do not reach it. CPU by default; `VOXIMA_TTS_CUDA=ON` builds the CUDA backend
(about 8-9 times faster than the CPU on an RTX 3070 Ti, where the CPU takes about 4 times real time with 4 threads).

Its WAV and safetensors files (test audio and the weights of models Voxima does not use, about 84 MB) were taken out
of the whole history, squashed commit included. Kept: `webui/native/demo_voices/*.wav` (its CMake reads them when
configuring) and the two its C API path test loads (`assets/framework/models/silero_vad/`,
`assets/resources/sample_16k.wav`). `git subtree pull --squash` would bring the rest back, in a commit that stays in
the history even if a later one deletes them. Update it by hand instead: check out audio.cpp at the new commit
elsewhere, copy its tree over `extern/audio.cpp` without those files (`rsync -a --delete --exclude .git`, then delete
the `*.wav` and `*.safetensors` but the ones above), build, test, and commit, naming audio.cpp's commit in the message.

`TTSClient` (`src/tts/`) calls audio.cpp's C API (`audiocpp.h`) in the same process:

- Speech: task `"clon"` (Chatterbox only offers voice cloning here), `audiocpp_request_set_voice_audio` with the
  reference clip. The model and session are loaded when `TTSClient` is made, so a bad model path fails when the
  render starts, not part way through. One session serves every language.
- Alignment: task `"align"`, `audiocpp_request_set_audio` with the speech. Word times come back in samples of the
  aligner's own 16 kHz front end, not the input's rate. Its confidence is always 0.
- Languages are passed as their primary subtag (`language::primary_subtag`: `en-GB` is spoken as `en`).
- Each line is cached as a `.wav` in `<render-dir>/tts-cache`, keyed by the text, voice, language, rate and model
  file name (so a new model does not reuse old audio). audio.cpp has no seed, so the cache is what makes a re-render
  sound the same.

## Voices

`voices::` in a script names a reference clip per narrator, relative to `--tts-voice-dir` (default `voices`). The
clips in `voices/chatterbox/` are all CC0 or public domain, with their sources in `voices/chatterbox/SOURCES.md`. Clips
that need credit (CC BY) are not kept, so a video cannot use one without credit by accident. Avoid Kyutai's Expresso
and EARS sets (CC-BY-NC).

## Timing

- `split_words()` (`utils/Words.h`) is the one definition of a word, used by both the narration and the text layout,
  so spoken word N is the `"word<N>"` group on screen.
- `Narrator` matches the aligner's words to those words when the counts agree; otherwise (no aligner, a failure, an
  unsupported language, a different count around numbers or hyphens) `distribute_words()` spreads them over the line
  by length.
- Markers (`{a}` in the script text, `utils/Markers.h`) are timed from the words, and are taken out before speech,
  alignment, subtitles and display.
- Several languages in one video: each language's narration is a `Timeline` (`audio/Timeline.h`) cut at the markers
  and line starts. `Narration::synchronise()` gives every segment the length of the slowest language and pads the
  others with silence (`Retime`). With `--max-speedup`, other languages may be shortened instead: pauses first, down to
  80 ms (`audio/Compress.h`), then the speech with ffmpeg's `atempo`, which keeps the pitch (`audio/Stretch.h`). The
  main language is never changed.
- Subtitle cues run from a line's first to last aligned word (the whole line without alignment). They are written as
  SRT and WebVTT next to the render, and muxed as `srt` (mkv), `mov_text` (mp4) or `webvtt` (webm).

## Known gaps

- **Chinese and Japanese** are not spoken: audio.cpp's Chatterbox language list (`model_specs/chatterbox.json`) leaves
  out `zh` and `ja`, though the original model has Chinese. Not patched locally (it would not survive a subtree
  update); an upstream issue would be the way.
- **Polish** is not in the aligner's language list, but aligned the sentences tried correctly. If it ever miscounts,
  the words fall back to the estimate.
- Only mkv subtitle muxing has been checked on a real render; mp4 and webm have not.
- `Narrator` and `Narration` have no tests that run without the real models (a fake `TTSClient` would allow it).
  The timing pieces under them (`Timeline`, `Retime`, `Compress`, `TimedWord`, word effects) are tested.
