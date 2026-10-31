# Third-party notices

Sonimation itself is under the MIT license (see `LICENSE`). It is built with the software below, which keeps its
own licenses. In an installed or packaged Sonimation, the full texts are in `share/doc/Sonimation/licenses/`.

## In this repository

| Component | Where | License | Text |
|---|---|---|---|
| audio.cpp (ShugoAI LLC): text to speech, built into `audiocpp` | `extern/audio.cpp` | Apache-2.0 | `licenses/audio.cpp/LICENSE` |
| ggml (part of audio.cpp) | `extern/audio.cpp/external/ggml` | MIT | `licenses/audio.cpp/ggml/LICENSE` |
| SentencePiece (part of audio.cpp) | `extern/audio.cpp/external/sentencepiece` | Apache-2.0 | `licenses/audio.cpp/sentencepiece/LICENSE` |
| Abseil, protobuf-lite, darts-clone, esaxx (part of SentencePiece) | `.../sentencepiece/third_party` | Apache-2.0, BSD-3-Clause, BSD-2-Clause, MIT | `licenses/audio.cpp/sentencepiece/<name>/LICENSE` |
| cJSON (part of audio.cpp) | `extern/audio.cpp/external/cJSON` | MIT | `licenses/audio.cpp/cJSON/LICENSE` |
| cpp-httplib (part of audio.cpp) | `extern/audio.cpp/external/cpp-httplib` | MIT | `licenses/audio.cpp/cpp-httplib/LICENSE` |
| libyaml (part of audio.cpp) | `extern/audio.cpp/external/libyaml` | MIT | `licenses/audio.cpp/libyaml/License` |
| llama_tokenizer (part of audio.cpp) | `extern/audio.cpp/external/llama_tokenizer` | no license file of its own; distributed with audio.cpp | |
| tree-sitter highlight queries for C and C++, compiled in | `extern/tree-sitter-queries` | MIT | `licenses/tree-sitter-queries/{c,cpp}/LICENSE` |

audio.cpp also carries some model weights and test recordings (in `extern/audio.cpp/assets`), which keep the
licenses of their original releases: see `extern/audio.cpp/docs/model_licenses.md`. They are not installed.

The reference voice clips in `voices/` are CC BY 4.0 or CC0, with their sources and attribution in
`voices/chatterbox/SOURCES.md`. They are not installed.

## Libraries from Conan

The libraries Sonimation is built against come from Conan (see `conanfile.py`), with what they depend on. Their
license texts, as each package ships them, are collected into `licenses/conan/<package>/` when Sonimation is
built, and installed with it. They include libraries under the LGPL (libsndfile, soxr, FriBidi, mpg123, LAME,
GLib, gettext, libiconv, libdwarf); their sources, and the recipes that build them, are on
[Conan Center](https://conan.io/center), and Sonimation's own source is public, so it can be rebuilt against
changed versions of them.

## Not included

The fonts (fetched by `download_fonts.py`) and the TTS models (see `tts.md`) are not part of Sonimation, and
keep their own licenses.
