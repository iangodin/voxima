# TTS System Overhaul Plan

Status: draft, research complete, implementation not yet started. Continuing next session.

## Why

The current narration pipeline (`src/tts/edge_tts_server.py` + `TTSClient`) is not
really "local TTS" — `edge-tts` calls out to Microsoft's cloud Edge TTS service (an
unofficial/reverse-engineered API), not a model we run ourselves. That means:

- Rendering requires network access.
- We depend on an undocumented API that can change or break without notice.
- Microsoft can silently update voices, so "the same" line can sound different over
  time even though our cache keys don't account for that.

Goal: replace it with a real local model that is deterministic/reproducible, covers
our target languages at good quality, and can be driven from C++ without a heavyweight
Python service.

## Known bug to fix regardless of TTS backend

`Narrator::speech()` (`src/core/Narrator.cpp:34-39`) only accumulates `duration` inside
the `if (result.audio->rate() != _audio_rate)` branch. When the TTS server's sample
rate already matches the mixer rate (the common case), `duration` stays `0.0` for that
result. This feeds animation timing and is very likely wrong today. Fix independent of
the rest of this plan.

## Chosen model: Chatterbox Multilingual (Resemble AI)

Decision: **Chatterbox only** (not Piper). Comparison that led here:

| Option | Languages | License | Verdict |
|---|---|---|---|
| **Chatterbox Multilingual v3** | 25 langs incl. pl/zh | **MIT** | Chosen. Natural/expressive, zero-shot voice cloning, active maintainer (Resemble AI). |
| Piper (`OHF-Voice/piper1-gpl`) | 30+, incl. en/fr/es/pl/zh | GPL-3.0 (relicensed after original MIT repo archived Oct 2025) | Safer/lower-risk fallback if Chatterbox integration stalls. Fixed fixed pretrained voices (not cloning) — deterministic by construction. Has official `libpiper` C/C++ API. |
| Kokoro-82M | 8 langs, **no Polish** | Apache-2.0 | Ruled out — missing a required language. |
| Supertonic 3 | 31 langs, Chinese coverage unclear | Unclear commercial terms; org appears archived | Ruled out — maintenance/license risk plus a possible language gap. |
| Coqui XTTS v2 | 17 langs incl. pl/zh | CPML (non-commercial-ish); company defunct, no path to a commercial license | Ruled out. |

Chatterbox variants: **Turbo** (350M, English-only, ~2x faster, ignores
`cfg_weight`/`language_id`) vs **Multilingual v3** (0.5B, 25 languages incl. dialects,
zero-shot cloning). We need Multilingual v3 for language coverage.

## Runtime: audio.cpp (ggml), not the Python reference implementation

[`0xShug0/audio.cpp`](https://github.com/0xShug0/audio.cpp) — canonical upstream,
Apache-2.0, pure C++ inference engine powered by ggml, no Python dependency. Has a
Chatterbox 0.5B backbone backend built in (GGUF, 16-bit/Q8).

Open items to verify in a validation spike — **all resolved, see "Phase 1 validation
spike: complete" below**:
- ~~Chinese language coverage is ambiguous~~ — **resolved: not supported.** Tested
  directly; hard error.
- **No documented determinism/seed controls** in audio.cpp. Mitigated in practice by
  our existing cache (`TTSClient.cpp`), which stores exact synthesized bytes per
  `(text, speaker, language, rate)` hash and reuses them forever — so *within a
  project*, output is already bit-reproducible once cached. What actually matters is
  **speaker-identity consistency** across different lines, which comes from using a
  fixed reference clip per named voice, not from model-level sampling determinism.
  (Not independently re-verified this session beyond the informal check under
  "Speaker-identity consistency" below — still no seed/determinism control exposed by
  the C ABI, unchanged.)
- ~~Confirm the GGUF conversion doesn't add license terms~~ — **resolved: clean.**
- ~~Measure CPU-only real-time-factor~~ — **resolved: ~4x RTF for realistic sentence
  lengths.** See benchmark below. GPU (CUDA, added 2026-09-22/23) is ~8-9x faster
  than CPU on the RTX 3070 Ti used for this work.

## Timing / subtitle sync

Chatterbox does not emit word boundaries during synthesis the way edge-tts's streamed
`WordBoundary` events did (autoregressive TTS models generally don't). audio.cpp ships
separate **forced-alignment models** that take our known script text plus the generated
audio and produce acoustically-grounded word-level timestamps as a second pass. This
plan originally named Meta MMS-300M for that role - **superseded, see "Phase 4 forced
alignment: complete" below**, MMS turned out to be CC-BY-NC-4.0 (non-commercial) and was
rejected; Qwen3 Forced Aligner (Apache-2.0) is what's actually wired in.

Implication: subtitle sync becomes a two-step pipeline per line (synthesize, then
align) instead of one. More robust in principle (grounded in the actual audio, not the
model's internal timing estimate), but it's an extra inference call and a new failure
mode to handle per line. The alignment pass itself is done (phase 4); SRT/WebVTT
generation and muxing are still open, scoped as part of the Subtitles task
(tasks.md `## Subtitles`).

## Integration approach: three options, leaning toward #1

audio.cpp added a **merged, opt-in C ABI** (PR #530, landed 2026-09-13):
`include/audiocpp.h`, C99, 69 entry points, gated behind `AUDIOCPP_BUILD_C_API`
(default OFF). Covers full TTS synthesis (not a subset), supports reusing a loaded
model/session across calls with flat memory over long runs (~40.5ms/request overhead
on CPU), handles can be freed out of order. Wraps audio.cpp's internal
`ModelRegistry -> ILoadedVoiceModel -> IVoiceTaskSession` layers without the caller
needing to touch ggml/CUDA headers directly.

1. **Direct C ABI, linked in-process.** `TTSClient` calls `audiocpp.h` functions
   directly — no thread, no socket, no process lifecycle. Fastest and simplest at the
   call site. Cost: pulls ggml (and optionally CUDA) into canim's own CMake build as a
   real dependency — `make config`/`make` would build a GPU-capable inference engine
   even for someone only touching math typesetting. Mitigate by making it an
   opt-in/off-by-default CMake component, so most contributors don't pay the build
   cost, and only render machines that need real narration enable it.
2. **In-process background thread hosting `audiocpp_server`**, talked to over
   localhost HTTP. Keeps the ggml/CUDA build dependency contained to one module/thread
   rather than every translation unit; canim still owns the process lifecycle (no
   external subprocess to babysit). Pays HTTP/JSON overhead per call (likely trivial
   next to inference time) and keeps a second protocol alive instead of retiring the
   client/server split entirely.
3. **Genuinely separate subprocess** (closest to today's shape). Most isolation,
   easiest to keep the CUDA toolchain optional for contributors who don't need it, but
   we manage spawn/health/shutdown ourselves (today's `edge_tts_server.py` requires
   manual startup — nothing auto-launches or health-checks it, a gap worth fixing
   regardless of which option we pick).

Leaning toward **option 1, as an optional CMake component off by default** — cleanest
call-site, and the opt-in build flag addresses the main objection (forcing ggml/CUDA
on every contributor). Not finalized — revisit after the validation spike.

If we go with the C ABI, `protocol.capnp` and the Python bridge (`edge_tts_server.py`,
`tts_client.py`, and the `test_*.py` scripts) can likely be retired entirely, along
with the `pycapnp`/`edge_tts` dependency chain.

## Reference voices (voice cloning needs a reference clip per named voice)

Chatterbox clones from a short reference sample rather than shipping fixed named
voices like edge-tts did, so we need to source one open, appropriately-licensed clip
per language/voice we want. Distinguish two consent stories:

- **Purpose-built voice donation**: someone recorded and released their voice
  specifically so it could be used for TTS/cloning. Strongest ethical footing.
- **Repurposed recording**: a recording licensed openly for some other original
  purpose (audiobook narration, research corpus) that happens to also permit this use
  under its license terms. Legally clear, but the speaker didn't necessarily
  anticipate being cloned by a TTS model.

| Language | Source | License | Consent story | Notes |
|---|---|---|---|---|
| en | [Voice-Zero](https://github.com/OwenTyme/voice-zero) (LibriVox and Archive.org narrators: Robert Middleton, Douglas Harlon, Bev J. Stevens, Emily Cripps) and `kyutai/tts-voices` `voice-donations/Midlands.wav` | CC0 / public domain | Repurposed (narration recordings); Midlands is a purpose-built donation | The English narrators since 2026-10-30, credited per voice in `voices/chatterbox/SOURCES.md`. Default: `en-voice-zero-robert-middleton.wav`. |
| fr, es, de, pl | LibriVox readers (Pauline Latournerie, Mario Pineda, Friedrich, Gosia Leksandrówna, Piotr Nater) and `kyutai/tts-voices` `voice-donations/SSA150803.wav` (fr) | Public domain / CC0 | Repurposed (audiobook narration); SSA150803 is a purpose-built donation | The narrators since 2026-10-30, credited per voice in `voices/chatterbox/SOURCES.md`. Defaults: `fr-kyutai-donation-ssa150803.wav`, `de-librivox-friedrich.wav`, `pl-librivox-gosia-leksandrowna.wav`; Spanish also keeps the CML-TTS clip below. |
| en (earlier) | [`kyutai/tts-voices`](https://huggingface.co/kyutai/tts-voices), `alba-mackenna/casual.wav` | **CC BY 4.0** (corrected 2026-09-23 — see below) | Purpose-built voice acting, attribution required | Checked in: `voices/chatterbox/en-alba-mackenna-casual.wav`, 48kHz, ~10s. |
| fr | `kyutai/tts-voices`, `cml-tts/fr/` subset | CC BY 4.0 | Repurposed (audiobook narration) | Checked in: `voices/chatterbox/fr-cml-tts-10087_11650_000028-0002.wav`, 24kHz, 7.24s. |
| de | [`Thorsten-Voice/TV-44kHz-Full`](https://huggingface.co/datasets/Thorsten-Voice/TV-44kHz-Full) (canonical HF mirror of [thorstenMueller/Thorsten-Voice](https://github.com/thorstenMueller/Thorsten-Voice)) | **CC0** (confirmed against both the HF card and the author's own GitHub README) | Purpose-built, explicit | Checked in: `voices/chatterbox/de-thorsten-voice-short.wav`, 24kHz, 5.48s. A separate third-party HF mirror, `Thorsten-Voice/TV-24kHz-Neutral`, mislabels the same recordings CC-BY-4.0 — don't use that one as the license source. Clip-length sensitivity tested (5.48s vs 13.29s from the same speaker/subset) — both produce valid, comparably-sized output; no functional break from a longer reference clip. |
| pl | [`freds0/cml_tts_dataset_polish`](https://huggingface.co/datasets/freds0/cml_tts_dataset_polish) (CML-TTS) | CC BY 4.0 | Repurposed | Checked in: `voices/chatterbox/pl-cml-tts-11295_11059_000062.wav`, 24kHz, 10.88s. |
| es | [`freds0/cml_tts_dataset_spanish`](https://huggingface.co/datasets/freds0/cml_tts_dataset_spanish) (CML-TTS) | CC BY 4.0 | Repurposed | Checked in: `voices/chatterbox/es-cml-tts-3719_1378_000081.wav`, 24kHz, 10.91s. |
| zh | **none — accepted gap, see below** | — | — | Not a licensing question: the model itself doesn't support Chinese through this integration (see "Phase 1 validation spike: complete"). |

**License correction (2026-09-23):** the `en` clip was originally logged as CC0. It
isn't — `kyutai/tts-voices`' own README states the `alba-mackenna/` collection
(voice-acted characters) is "Released under the CC BY 4.0 licence", distinct from
`voice-donations/` (the actual CC0 subset, but with anonymized/unlabeled-by-language
filenames, not practical to pick a specific language from). Fixed in
`voices/chatterbox/SOURCES.md`; still fine to use, just requires attribution (Alba
MacKenna, via kyutai/tts-voices) rather than being license-free.

Explicitly avoid: Expresso and EARS from the Kyutai collection — **CC-BY-NC**,
non-commercial only.

Plan: pick one reference clip per language/voice, check the clip + a short license/
source note into the repo (or a documented fetch step), replacing the current
`voices` config in `Script` (currently a plain edge-tts `ShortName` string) with a
reference-clip path per named voice.

## Rough phase breakdown (not yet scheduled)

1. **Validation spike — complete, see "Phase 1 validation spike: complete" below.**
2. **Backend swap — complete, see "Phase 2 backend swap: complete" below.**
3. **Carry over fixes identified along the way — done as part of phase 2:**
   - ~~Fix the `Narrator::speech()` duration bug.~~ Fixed.
   - ~~Add retry/backoff and a fail-fast reachability/availability check before a render
     starts.~~ Superseded: there's no separate server process to health-check anymore
     (audio.cpp is in-process), so the equivalent is eager model/session load at
     `TTSClient` construction time - a bad model path throws at startup, not mid-render.
     Retry/backoff no longer applies to an in-process library call the way it did to a
     flaky network server.
   - ~~Version the cache key (include a model/backend identifier).~~ Done - the cache
     hash now folds in the model file's name.
4. **Subtitle sync — complete, see "Phase 4 forced alignment: complete" below.** Scoped
   strictly to wiring the alignment pass into `TTSClient`; SRT/WebVTT generation and
   muxing are NOT part of this phase and remain unimplemented - still open in
   `tasks.md`'s `## Subtitles` task.
5. **Testing** — add a mock/fake TTS backend mode so `Narrator`/`Narration` logic gets
   ctest coverage without requiring the real model.

## Decisions made 2026-09-22

- **TTS is now a required dependency, not optional.** Dropping the earlier
  "off-by-default CMake component" hedge — audio.cpp/ggml gets linked into every
  build of `visuals`, same as lunasvg/harfbuzz/etc. Simplifies the CMake story at the
  cost of every contributor paying the ggml build time.
- **CPU-only for the initial proof of concept.** No CUDA/HIP/Vulkan/Metal enabled yet.
  Verified via audio.cpp's README/CMakeLists: CPU-only is a first-class, fully
  documented path (`ENGINE_ENABLE_{CUDA,HIP,VULKAN,METAL}=OFF`), not an afterthought.
  Metal (macOS) is supported upstream (`ENGINE_ENABLE_METAL` + `xcrun`) but explicitly
  deferred — no Mac available to test on right now.
- **Cross-platform target confirmed: Linux, macOS, Windows.** audio.cpp documents
  toolchains for all three (GCC 13+/CMake on Linux, VS Build Tools 2022+/MSVC/Ninja on
  Windows, Xcode CLT/CMake on macOS). Windows/macOS builds untested by us so far.
- **Vendoring mechanism: `git subtree add --squash`, not a submodule.** Verified
  audio.cpp has exactly one nested git submodule of its own
  (`external/audio.cpp-server-frontends`, a web UI we don't need for a CLI/library
  integration) — everything else needed (`ggml`, `sentencepiece`, `cJSON`, `libyaml`,
  `llama_tokenizer`) is vendored directly under `external/` in-tree, so a squashed
  subtree pulls in a complete, buildable source tree without submodule-of-submodule
  headaches. Target path: `extern/audio.cpp` (matches the include dir already wired
  into the top-level `CMakeLists.txt`).
- **Working branch: `new_tts`.** Land the proof of concept here before attempting
  full `TTSClient`/`Narrator` integration.

## Verified audio.cpp build requirements (2026-09-22)

- Toolchain: CMake >= 3.20, GCC 13+ (Linux) / MSVC + VS2022 Build Tools + Ninja
  (Windows) / Xcode CLT (macOS). C11 / C++17 internally (audio.cpp's own standard;
  canim itself stays C++20).
- Required system libs: POSIX Threads. Optional: OpenMP (CPU parallelism), Git (build
  metadata), Python3 (test validation only).
- CPU-only config flags: `ENGINE_ENABLE_CUDA=OFF -DENGINE_ENABLE_HIP=OFF
  -DENGINE_ENABLE_VULKAN=OFF -DENGINE_ENABLE_METAL=OFF` (native CPU codegen +
  llamafile SGEMM kernels stay on by default).
- C ABI (`AUDIOCPP_BUILD_C_API=ON`): builds `engine_runtime` + `ggml` statically into
  one shared object exporting only `audiocpp_*` symbols; needs Python3 for export
  surface tests.
- Models/voices are **not** a build dependency — fetched separately at
  setup/run time via `tools/model_manager_v2.py` or `audiocpp_model_manager`, from HF
  Hub or a GGUF repo. Reference voice clips (small, CC0/CC-BY) get checked directly
  into the repo per the table above.

## Proof-of-concept scope (before any Narrator/TTSClient integration)

Small standalone CLI tool, `tools/tts_poc.cpp` (or similar): takes a plain text file
as input, synthesizes English speech via the Chatterbox Multilingual GGUF backend
through audio.cpp's C ABI, writes a `.wav` file. CPU only. No forced-alignment/timing,
no multi-language, no voice-cloning-per-named-voice yet, no Narrator/Script wiring —
purely "does the engine work end to end on our toolchain."

## Proof of concept: done (2026-09-22)

`tools/tts_poc.cpp` builds and runs end to end on Linux, CPU only:

```
make config && make
./build/Release/tts_poc <text_file> models/Chatterbox-GGUF/chatterbox-q8_0.gguf \
    voices/chatterbox/en-alba-mackenna-casual.wav <out.wav>
```

Produced 8-9s of audible English speech (peak ~27000/32768, not silent) from a
123-character input sentence, cloning the reference voice clip. Wall time ~29s
with 4 threads on this machine (`audiocpp_backend_config.threads`), i.e. CPU RTF
is roughly 3-3.5x real time for a Q8_0 quantized 0.5B model on this
hardware/thread count - not yet benchmarked systematically or tuned (thread
count, quantization level) for a render-machine target.

What this validated:
- audio.cpp's C ABI (`audiocpp.h`) works as described: `registry_create` ->
  `model_load` (`family_hint = "chatterbox"`) -> `session_create` (task token
  is **`"clon"`**, not `"tts"` - Chatterbox only exposes voice-clone/voice-
  conversion in this integration, confirmed against `docs/tts.md` and
  `task_vocabulary.cpp`; there is no built-in-voice TTS path for the base
  Chatterbox family, only for `chatterbox_turbo`) -> `request_create` +
  `set_text` + `set_voice_audio` -> `session_run` -> `result_audio`.
- CPU-only build config (`ENGINE_ENABLE_{CUDA,HIP,VULKAN,METAL}=OFF`,
  `AUDIOCPP_BUILD_C_API=ON`) builds cleanly once canim's own
  `-Werror -Wall -Wextra` is kept from leaking into the vendored subdirectory
  (directory-scoped `add_compile_options` in CMake applies to subdirectories
  added afterward - fixed by moving `add_subdirectory(extern/audio.cpp)`
  before that block in the top-level `CMakeLists.txt`).
- `AUDIOCPP_MODEL_SET=custom` / `AUDIOCPP_MODELS=chatterbox` restricts the
  vendored build to just the one model family instead of audio.cpp's full
  ~90-family default, which matters a lot for contributor build time.
- Chatterbox Q8_0 GGUF weights: 2.09 GB, fetched from
  `audio-cpp/audio.cpp-gguf` on HF Hub via `huggingface_hub.hf_hub_download`
  (no `model_manager_v2.py`/native manager needed for a single-file fetch).
  Live under `models/` (gitignored, not committed).
- Reference voice clip: `voices/chatterbox/en-alba-mackenna-casual.wav`
  (`kyutai/tts-voices`, CC0, purpose-built donation) - **checked into a new
  top-level `voices/` directory**, not `assets/` (the existing `assets/` is
  entirely gitignored project-wide, for downloaded/generated content like
  fonts - not the right place for something meant to be tracked). Source/
  license noted in `voices/chatterbox/SOURCES.md`.

Deviations from the original integration sketch:
- TTS is now unconditionally required (per this session's decision), so
  `extern/audio.cpp` is `add_subdirectory`'d unconditionally in the top-level
  `CMakeLists.txt`, no opt-in CMake flag.
- Confirmed `git subtree add --squash` (not submodule) works cleanly:
  `extern/audio.cpp` is a normal, complete, buildable source tree (~223MB).
  The one upstream submodule it references (`audio.cpp-server-frontends`) is
  absent/empty post-subtree and is harmless - it's only needed for
  `AUDIOCPP_BUILD_SERVER_FRONTENDS` (off by default, not used here).

Not yet exercised by the POC: multi-language, forced alignment/timestamps,
`Narrator`/`TTSClient`/`Script` integration, Windows/macOS builds, voice
selection beyond one hardcoded reference clip, cache-key versioning.

## Phase 1 validation spike: complete (2026-09-23)

All remaining phase-1 items closed out. Summary (detail folded into the relevant
sections above):

- **License chain: confirmed clean.** `extern/audio.cpp/LICENSE` is Apache-2.0
  (ShugoAI LLC). The specific `Chatterbox-GGUF` package in the
  `audio-cpp/audio.cpp-gguf` HF repo we pull `models/Chatterbox-GGUF/chatterbox-q8_0.gguf`
  from is listed as MIT in that repo's own README license table, matching
  `ResembleAI/chatterbox` upstream (also independently confirmed MIT via its HF
  model card). The GGUF repo's own top-level HF tag is `license: other` only
  because it's a multi-model bundle covering ~80 different model families each
  under their own license — its README states explicitly "[u]se and
  redistribution are governed by the corresponding original model license", i.e.
  no additional terms are added by the GGUF conversion itself for our file.
- **Chinese (`zh`): definitively not supported, not a licensing question.**
  Directly tested: `tts_poc <chinese text file> ... zh` fails immediately with
  `Error: run: unsupported Chatterbox language: zh` — a hard rejection from
  audio.cpp's Chatterbox backend, not silently-wrong output. Matches
  `extern/audio.cpp/model_specs/chatterbox.json`'s `languages` list, which
  omits `zh` even though upstream `ResembleAI/chatterbox`'s own HF model card
  tags list `zh` among 29 supported languages — the discrepancy is real:
  Resemble's original model supports Chinese, audio.cpp's Chatterbox GGUF
  integration currently does not expose it. **Decision needed from you:**
  accept this gap for now (per the plan's own originally-listed option), or
  pursue it upstream (file an audio.cpp issue/PR; the `cangjie_mapping`
  asset referenced in the model spec's `sources.files` suggests some Chinese
  g2p scaffolding already exists in the integration, just not wired to the
  language allowlist — may not be a large fix, but this wasn't investigated
  further this session).
- **fr/de/es/pl reference voices: sourced and checked in.** See the updated
  "Reference voices" table and `voices/chatterbox/SOURCES.md` above. Also
  found and fixed a real license mislabel on the existing `en` clip (was
  logged CC0, is actually CC BY 4.0) while re-verifying the `en` source.
- **Multi-language synthesis smoke test: 4/4 pass.** fr/de/es/pl each
  produced non-silent, sane-duration audio from a representative sentence
  (peaks 5982-15285 of 32768, durations 3.4-4.5s for similar-length inputs).
  Naturalness/correctness not judged (no listener) — only "did it produce
  real audio, not silence or a crash."
- **Speaker-identity consistency: 3/3 runs completed normally.** Three
  different English sentences (short/medium/long) synthesized from the same
  `en-alba-mackenna-casual.wav` reference all produced plausible,
  duration-proportional audio. This only checks "the pipeline doesn't
  break/degrade across repeated use of one reference clip" — actual
  perceptual speaker-identity consistency needs a human listener and is not
  verified here.
- **Thorsten-Voice clip-length sensitivity: no functional issue.** 5.48s vs
  13.29s reference clips from the same speaker both produced valid,
  comparable-duration output for an identical input sentence. Again,
  functional only, not a perceptual quality comparison.
- **CPU RTF benchmark (RTX-class CPU, `backend=cpu` forced, `--threads 4`):**

  | Input | Chars | Synth time | Audio duration | RTF |
  |---|---|---|---|---|
  | short | 11 | 11.83 s | 1.22 s | 9.7x |
  | medium | 77 | 19.65 s | 4.66 s | 4.2x |
  | long | 264 | 63.29 s | 15.9 s | 4.0x |

  RTF converges to **~4x real time** for realistic narration-length
  sentences; the short-input outlier (9.7x) reflects fixed per-request
  overhead dominating a tiny output, not a per-character cost difference.
  For comparison, **CUDA (RTX 3070 Ti, confirmed 2026-09-22) is ~8-9x faster
  than this** — ~2.5-3.3s synthesis for the same 97-char benchmark sentence
  that takes CPU ~24-26s, i.e. roughly real-time-or-faster on GPU vs ~4x
  slower-than-real-time on CPU. Thread-count sweep and Q8_0-vs-F16 precision
  comparison not done this session — the Q8_0/4-thread numbers above are
  what a typical contributor machine would get.

## Decision 2026-09-23: Chinese gap accepted

Accepting the `zh` gap for now rather than blocking on an upstream fix to
`0xShug0/audio.cpp`'s `model_specs/chatterbox.json` language allowlist. Not
pursuing a local patch to our vendored `extern/audio.cpp` copy either — too
fragile against future subtree updates, and the underlying g2p support may be
disabled for a real reason we can't see from outside. Worth filing an issue
upstream at some point, but not blocking phase 2 on it. Revisit if/when
Chinese narration becomes an actual near-term requirement.

**Phase 1 (validation spike): complete.**

## Phase 2 backend swap: complete (2026-09-24)

`TTSClient` (`src/tts/TTSClient.h`/`.cpp`) now calls audio.cpp's C ABI in-process
instead of talking capnp-over-socket to `edge_tts_server.py`. Design:

- **Eager load, not lazy.** The constructor loads the Chatterbox GGUF and creates a
  synthesis session immediately (`audiocpp_registry_create` -> `audiocpp_model_load`
  -> `audiocpp_session_create` with task token `"clon"`). A missing/bad model path
  throws at `DubbingStage::setup()` time, i.e. at render startup, not on the first
  `synthesize()` call mid-render. This is the fail-fast mechanism that replaces phase
  3's "retry/backoff and reachability check" item - there's no longer a separate
  server process to health-check or retry against, so that item doesn't carry over
  in its original shape.
- **One model + one session, shared across all languages.** `DubbingStage` owns a
  single `TTSClient`; every per-language `Narrator` calls into the same loaded
  session. Proven end-to-end (not just assumed from audio.cpp's docs) by a real
  render through `Producer`/`DubbingStage`/`Narrator` with `--lang en --lang fr`:
  both languages synthesized correctly from the one persisted session, produced
  non-silent audio (en: 3.54s, mean -20.5dB; fr: 3.54s, mean -25.1dB) and a working
  file cache (one `.wav` per unique text/voice/language hash).
- **`voices::` config is now a reference-clip path**, not an edge-tts ShortName.
  Resolved by `Narrator` against a `--tts-voice-dir` base directory (default
  `voices`) supplied via `DubbingStage::setup()`. `demos/eases/*.huml` updated for
  en/fr/de (the languages with committed clips); `en-GB`/`fr-CA` now inherit their
  base language's clip instead of carrying a stale edge-tts-only override; `zh`/`jp`
  get an explicit comment that narration will fail for them (see the `ja` gap below).
- **Cache format**: switched from raw cached capnp response bytes to a decoded
  `.wav` per hash (`TTSClient`'s own cache, distinct from the model/voice files).
  The hash now folds in the model file's name, so swapping GGUFs automatically
  busts stale entries instead of silently serving audio from a different model.
- **`Narrator::speech()` duration bug fixed** - `duration` now accumulates
  unconditionally per result instead of only inside the resample branch.
- **New CLI options** on `sonimation`: `--tts-model` (default
  `models/Chatterbox-GGUF/chatterbox-q8_0.gguf`), `--tts-voice-dir` (default
  `voices`), `--tts-backend` (default `best`, i.e. CUDA if built with it), `--tts-threads`
  (default 4).
- **New gap discovered**: Japanese (`ja`) is unsupported by this Chatterbox
  integration too, same as `zh` - confirmed against `model_specs/chatterbox.json`'s
  language allowlist (ar, da, de, el, en, es, fi, fr, hi, it, ko, ms, nl, no, pl, pt,
  sv, sw, tr - no `zh`, no `ja`). Folded into the same accepted-gap decision as
  Chinese; `demos/eases/jp.huml` flagged with a comment, not a functional blocker
  since nothing renders it today.
- **Retired**: `edge_tts_server.py`, `tts_client.py`, the `test_*.py` scripts,
  `protocol.capnp` and its generated sources, `asio_wrapper.h`, and
  `tools/test_tts.cpp` (superseded by `tools/tts_poc.cpp`). Dropped the `CapnProto`
  and `asio` conan/CMake dependencies entirely - nothing else in the tree used
  either. `word_boundaries`/`WordBoundary` also dropped from `SynthesisResult`:
  Chatterbox doesn't produce them and nothing outside `TTSClient` consumed the field.
- **`rate` parameter**: kept in `TTSClient::synthesize()`'s signature for API
  compatibility, forwarded to `audiocpp_request_set_speaking_rate()` only when
  non-default and the loaded model advertises style-conditioning support
  (`audiocpp_model_supports_style_condition`). Not exercised by anything today -
  `Narrator` always calls with the default 1.0.
- `make test`: 5/5 pass, no regression from the dependency swap.

## Phase 4 forced alignment: complete (2026-09-24)

**Model: Qwen3 Forced Aligner (`qwen3_forced_aligner`, 0.6B, GGUF), not Meta MMS-300M
as originally planned.** MMS is disqualified: its checkpoint is CC-BY-NC-4.0
(non-commercial - same rejection category as Coqui XTTS was in the original model
comparison), audio.cpp lists it `"status": "community"` with "GGUF conversion is
local-only, no public redistribution is approved" (no clean way to even fetch weights
the way we did for Chatterbox), and its native text normalization only really covers
nl/en. Qwen3 Forced Aligner is `"status": "supported"`, license verified against
`extern/audio.cpp/docs/model_licenses.md`: **Apache-2.0**, redistributable, no
caveats - consistent with audio.cpp's own Apache-2.0 and Chatterbox's MIT weights.

**Language coverage**: 11 languages (`zh, en, yue, fr, de, it, ja, ko, pt, ru, es`).
Covers en/fr/de/es of our five synthesis languages. **Polish has no alignment support**
- accepted as a gap, same category as the zh/ja synthesis gaps from phase 2 (not
pursued further this session).

**API**: `TTSClient::align(text, audio, language) -> std::vector<WordTimestamp>`
(`src/tts/TTSClient.h`), where `WordTimestamp` is `{ word, start_seconds, end_seconds,
confidence }` - a deliberate callback to the `WordBoundary` struct phase 2 dropped
(that was edge-tts's live streamed estimate; this is grounded forced-alignment
instead). Unlike Chatterbox, the aligner model loads lazily on first `align()` call,
not eagerly at `TTSClient` construction - most renders won't need subtitle timing and
shouldn't pay for a second model load. `align()` throws if no `aligner_model_path` was
given at construction.

**Implementation notes**:
- Uses audio.cpp's `"align"` task token, `audiocpp_request_set_audio()` (not
  `set_voice_audio`, which is for cloning references) with the already-synthesized
  PCM, and `audiocpp_result_word_count()`/`audiocpp_result_word()` for the per-word
  results.
- The aligner reports `start_sample`/`end_sample` in units of its own internal
  frontend's sample rate (a Whisper-style 16 kHz frontend, per
  `extern/audio.cpp/src/models/qwen3_asr/frontend_whisper.cpp`, which the aligner
  shares its frontend design with - see `extern/audio.cpp/src/models/
  qwen3_forced_aligner/session.cpp`), **not** the input audio's rate;
  `audiocpp_request_set_audio()` resamples internally. This isn't documented as a
  public constant, so it was confirmed empirically: synthesized audio at Chatterbox's
  native rate, ran `align()`, and verified the last word's end time landed inside the
  known audio duration (it did, both in English and French - see below).
- `audiocpp_result_word()`'s confidence output is **always 0.0** for this model -
  confirmed in audio.cpp's own source (`qwen3_forced_aligner/processor.cpp:347` hardcodes
  `timestamp.confidence = 0.0F`). Not a bug in our integration; the field exists in the
  C ABI generically but this model doesn't populate it.
- Build: added `qwen3_forced_aligner` to `CMakeLists.txt`'s `AUDIOCPP_MODELS` custom
  set alongside `chatterbox`. Confirmed via the configure-time "model composite"
  message that this pulls in `qwen3_asr` as a linked dependency (shared frontend code
  between the aligner and ASR families) but *not* `qwen3_tts` or audio.cpp's wider
  ~90-family default set - acceptable scope.
- New tool: `tools/align_poc.cpp` (mirrors `tools/tts_poc.cpp`) - synthesizes a line,
  runs `align()`, prints word timestamps. Used for the verification below.
- Aligner GGUF (`models/Qwen3-ForcedAligner-0.6B-GGUF/qwen3-forced-aligner-0.6b-q8_0.gguf`,
  same `audio-cpp/audio.cpp-gguf` HF repo as Chatterbox) lives under `models/`
  (gitignored), not committed.

**Verification (real output, not just "it didn't crash")**:

English, "Hello there, this is a test of forced alignment timing." (10 words, 3.7s
audio): 10 words returned, monotonically increasing, last word ends at 3.36s (inside
the 3.7s audio) - `Hello [0.160-0.480]` ... `timing [2.960-3.360]`.

French, "Bonjour, ceci est un test de synchronisation." (7 words, 3.68s audio): 7 words
returned, monotonically increasing, last word ends at 3.2s (inside the 3.68s audio) -
`Bonjour [0.080-0.640]` ... `synchronisation [2.160-3.200]`.

Both: word count matches the input sentence exactly, timestamps strictly increasing,
per-word durations plausible for natural speech pacing (not all-zero, not degenerate).

**Not in scope for phase 4** (deferred to `tasks.md`'s `## Subtitles` task): SRT/WebVTT
file generation, muxing subtitle tracks into the rendered output, and wiring `align()`
into `Narrator`/`Narration`'s actual render loop - this phase only proves the alignment
capability itself works end to end via the standalone `TTSClient` API and `align_poc`.

`make test`: 5/5 pass, no regression.

## Subtitles task complete (2026-09-23)

Wired `align()` into the real render pipeline and built SRT/WebVTT generation +
muxing, closing out `tasks.md`'s `## Subtitles` task and the "mix the multiple
subtitle tracks" item under `## Final Muxing`. This was scoped separately from phase
4 (which only proved the alignment capability itself works, via the standalone
`TTSClient` API and `align_poc`).

**Timing design - the frame-counter anchor.** The naive approach (cumulative
duration of narrated lines) is wrong whenever narration doesn't start exactly at the
top of a render: `Narrator::speak()` is called once per video frame (in lockstep with
`DubbingStage::write_audio()`, which writes every language mixer's buffer - silence
included - once per Producer tick), so a line's *absolute* position in the final
output audio depends on which frame narration happened to land on, not just how long
prior lines were. Fixed by giving `AudioMixer` a running `frames_written()` counter,
incremented exactly where `write_audio()` already writes to the sndfile. `Narrator`
captures this counter (via its existing `Microphone` handle) at the first `speak()`
call of a new narration batch - before that frame's `write_audio()` runs, so it's the
exact absolute sample position that frame's audio will land at - and uses it as the
anchor to convert each line's local (batch-relative) cue span into absolute time.
This is accurate to one video frame (~1/24s at the demo's frame rate), not
sample-exact - a deliberate simplification; captions don't need sub-frame precision,
and chasing it would need much deeper plumbing for no visible benefit.

**Per-line cue span**: forced alignment (`_tts->has_aligner()` + `align()`) gives the
first/last word's start/end, trimming leading/trailing silence Chatterbox left in;
falls back to the full synthesized-buffer duration `[0, duration]` when no aligner is
configured, or `align()` throws/returns nothing (the Polish/zh/ja accepted-gap case) -
subtitles still get generated for those languages, just without silence trimming.

**Ownership**: cues are owned by `DubbingStage` (new `SubtitleCue` struct, a
per-language `std::vector`, pushed via `DubbingStage::caption()`), not by the
Scene-local `Narrator`/`Narration` objects - mirrors exactly how audio output is
already owned there. `Narrator::speech()` now accumulates cues across every call for
the render's lifetime (a render can have multiple narration scenes), rather than
clearing them like `_results` does for playback state.

**File formats and muxing**: new `src/exporters/SubtitleWriter.{h,cpp}` writes both
SRT (`HH:MM:SS,mmm`) and WebVTT (`HH:MM:SS.mmm`, `WEBVTT` header) - both always
written as sidecar files (`render/subtitles.<lang>.srt` / `.vtt`), satisfying the
"generate SRT" and "generate WebVTT" checkboxes as real, separate deliverables
regardless of container. `FfmpegMux` gained `add_subtitle_track()`; the actual
subtitle codec is picked per output container in `Producer.cpp`'s `setup_codecs()`
(mp4 -> `mov_text`, fed from the `.srt` sidecar, since mp4 has no other broadly
compatible option; mkv -> `srt` natively, fed from `.srt`; webm -> `webvtt`, fed from
`.vtt`, since webm doesn't accept SRT input at all). A language with zero cues is
skipped cleanly, not treated as an error - e.g. a render with no narration, or a
render using only zh/ja (unsupported for synthesis entirely, so no audio and no cues
either).

New CLI option: `--aligner-model` (default
`models/Qwen3-ForcedAligner-0.6B-GGUF/qwen3-forced-aligner-0.6b-q8_0.gguf`), threaded
through `DubbingStage::setup()` to `TTSClient`'s existing (previously unused in the
real call site) `aligner_model_path` constructor argument.

**Real end-to-end verification** (not just unit-level): temporarily re-enabled
`demos/eases/show_animation.cpp`'s narration scene (same approach phase 2's work used
- verified, then reverted back to the original committed/commented-out state
afterward) and ran the actual `Producer`/`DubbingStage`/`Narrator` pipeline via
`render.sh`'s invocation pattern (`--lang en --lang fr`, mkv container). Real output:

```
render/subtitles.en.srt:
1
00:00:00,240 --> 00:00:02,480
These show various ease animations

render/subtitles.fr.srt:
1
00:00:00,080 --> 00:00:02,720
Ceux-ci montrent diverses animations de lissage.
```

`ffprobe` on the muxed `render/final.mkv` confirmed real subtitle streams, not just a
successful exit code:

```
[STREAM] index=3 codec_name=subrip codec_type=subtitle TAG:language=en [/STREAM]
[STREAM] index=4 codec_name=subrip codec_type=subtitle TAG:language=fr [/STREAM]
```

Cue timing is plausible (starts near the render's start, ~2.3-2.6s spans matching the
synthesized audio duration for that one-line scene). mp4/webm codec selection was
implemented per the same table above but not independently smoke-tested with a real
render this session (only mkv was) - low risk since it's a straightforward per-format
lookup, but worth a quick real check before relying on it for those containers.

`make test`: 5/5 pass, no regression.

## Fixed 2026-09-23: Chatterbox rejected regional language variants

`Narrator` passed whatever raw `--lang` tag was requested (e.g. `en-GB`, `fr-CA`)
straight through to `TTSClient::synthesize()`/`align()`. Chatterbox's language
allowlist is bare ISO 639-1 primary subtags only, so a render with `--lang en-GB` or
`--lang fr-CA` - both of which already have real demo scripts - hard-failed with
`unsupported Chatterbox language: en-GB`, even though the base language is fully
supported; only the tag was wrong. Fixed as part of `tasks.md`'s "Handle BCP-47 / ISO
639 conversion" task: a new `language::primary_subtag()` utility (`src/utils/Language.h`)
extracts the normalized primary subtag for TTS/container-metadata use, while
`Narrator::language()`/script lookups keep the full requested tag. Confirmed via a real
render (`--lang en-GB`, reverted the temporarily-uncommented demo narration scene
afterward, per the usual verification pattern this session) - synthesizes correctly,
mux metadata now reads `language=en` instead of the invalid `en-GB`.

## Word-level animation sync (2026-09-24)

The forced-alignment pass (phase 4) used to feed only subtitle cues: `Narrator` kept the
first word's start and the last word's end and threw the rest away. The per-word times
now reach the animation code, so visuals can be driven by what is being said.

**One definition of "a word".** `split_words()` (`src/utils/Words.h`) is shared by the
narration timing and the text layout, so spoken word N is word N on screen. Whitespace
separates words, punctuation stays attached, no-break spaces glue, tokens with nothing
pronounceable (a lone dash, a quote) are not words, and Han/kana are split per character
(no dictionary segmentation - a documented approximation of UAX #29, not ICU).

**Timing** (`src/audio/TimedWord.h`, `Narrator`). `Narrator::words()` gives a
`TimedWord { word, line, begin, end, start_seconds, end_seconds }` per word, relative to
the start of the `speech()` batch, across all its lines. `match_words()` pairs the
aligner's words with `split_words()` when their counts agree. Otherwise - no
`--aligner-model`, an alignment failure or unsupported language, or a tokenization
mismatch around numbers/hyphens - `distribute_words()` spreads the words over the line
in proportion to their length in code points. Subtitle cues are unchanged and still use the
aligned first/last word, or the whole line as a fallback.

`Narrator::position()` is the seconds of the batch's audio consumed by `speak()` so far.
Word times and position are both batch-relative, so no absolute anchor is needed;
animation lands within a frame of the audio.

**API** (`Narration`, master language only - other tracks have different timings):

```cpp
narration.speech( "intro" );
co_await narration.until_word( 3 );       // resumes when word 3 starts
co_await narration.until_word_end( 3 );
co_await narration.until_time( 1.5 );     // seconds since the batch started
co_await narration.on_words( text, []( Visual &word, size_t index ) { ... } );
```

`track_time( from, to, cb )` / `track_words( first, last, cb )` call `cb( t )` every frame with how far the
narration is through the span (0 to 1, the last call exactly 1). They drive something that should last as long
as the words do, e.g. a morph: `co_await narration.track_words( 2, 3, [&]( float t ) { morph.set( t ); } )`.
With `morph_driven( stage, from, to, options, drive )` (`core/MorphAnim.h`) a shape can change on the words
being said (the demo's `follow_speech`). Morphing text: `LeafMatch::AUTO` matches whole words first (identical
words kept in reading order, the words between paired in order), so "the big cat" -> "the cat" fades only "big".

**Per-word effects.** `narration.animate_words( text, effect, first_word = 0 )` runs an effect on every word of a
laid-out text, each starting when its word is said. An effect is a coroutine over a `WordContext` (the word's
visual, its text, its timing, `bounds()`, `center()`, and `over_word( fn )` / `over_time( from, to, fn )`, which
call `fn( t )` with t from 0 to 1), so it keeps its state in local variables and cleans up after itself
(`core/WordEffects.h`). Ready-made ones in `word_effects::`: `reveal()`, `color( before, during, after )`, `pop( scale )`,
`underline( color )`, combined with `all( { ... } )`. The setup at the top of an effect runs as soon as
`animate_words` starts, so words can be hidden or dimmed until they are said. The context carries the narration
clock as a function, so effects are unit-tested with a fake clock (`tests/TestWordEffects.cpp`).

`speak()` must still be called each frame (the demo's `narrate()` loop), typically in a
`choreograph()` alongside these. They return at once if nothing is being narrated.

**Markers (multi-language sync).** Word numbers do not carry over between languages (sentences differ in
word count, words in length), so cue points are written into the script text instead, in every language, where
they correspond: `These show {a}various ease{b} animations.` / `Ceux-ci montrent {a}diverses animations{b} de
lissage.` (`utils/Markers.h`: `{name}`, name = letters, digits, `_`, `-`; `{{` and `}}` are literal braces; other
braces are text; a marker between two spaces leaves one). They are taken out before synthesis, alignment,
subtitles and display (`strip_markers()` for text shown on screen). `Narrator` times each one from the word
alignment (`time_at_offset`: the start of the next word; inside a word, how far through it in letters; at the end,
when the speech ends), or from the proportional estimate where there is no alignment. `Narration` exposes them for
the main language: `marker_time( name | index )`, `until_marker`, `track_markers( from, to, cb )`,
`track_to_end`, `duration()`. `speech()` warns on stderr if a language's markers differ from the main
language's (missing, extra, or in another order). Verified on real renders: a shape driven by `{a}`..`{b}` starts
and ends at the same offsets from the markers in English and French, although the markers fall at different times.

**Keeping languages in step (one video, several audio tracks).** Each language's narration is a `Timeline`
(`audio/Timeline.h`): cut at every marker and at the start of every line after the first (the cut for a marker is up
to 40 ms ahead of the word, no more than half the pause before it, so the first sound is not clipped). Every
language then has the same segments, each a different length. `Narration::synchronise()` gives each segment the length of the
slowest language, starts the segments at the same moments, and `Narrator::retime()` rebuilds each language's audio
with silence at the end of every shorter segment (`retime_audio`, with a 2 ms fade only where silence is added
next to speech) and moves its words, markers and subtitle cues the same way. A marker then falls at the same time
in every language (a language's own lead before the word, at most 40 ms, is the only difference). Languages whose
markers or lines differ from the main one's cannot be kept in step and are left as they are (with a warning).
With `--max-speedup S` (default 1.0) the other languages may be speeded up: a segment is as long as the main
language takes, or the others take at their fastest (`length / S`), whichever is more, and the main language is never
speeded up (`barrier_lengths( timelines, main, S )`). A segment given less time than a language needs goes through
`compress_segment` (`audio/Compress.h`): each pause is shortened, down to 80 ms, in proportion to what it can spare,
before anything else; only the little still missing is taken from the speech, by `stretch_with_ffmpeg` (ffmpeg's
`atempo`, which keeps the pitch; a `Stretcher` so tests use a fake). A `TimeMap` per segment moves the words, markers
and subtitle cues through the shortened segment. Verified on a real render at `--max-speedup 1.25`: the timeline
went from 6.88 s to 5.88 s, English (main) stayed identical to its natural audio in every segment, and French's
shortened segments kept their pitch (240 to 239 Hz, 207 to 209, 169 to 169, 198 to 198). Verified on a real render: every segment of each
retimed track equals its natural audio to within 16-bit rounding, sits at the shared start, and the padding is exactly
silent; the animation follows the main language's markers.

**Text side.** `make_styled_text()` groups each word's glyphs into a `Group` tagged
`"word<N>"` inside its `TextLine` (N counts through the whole text, across lines);
`find_word( root, N )` looks it up. Glyphs are assigned by HarfBuzz cluster, so RTL words
and words split by style ranges work, and math-alphabet remapping is accounted for. Word
groups sit at the origin, so glyph positions are unchanged. This requires the on-screen
text to be the narrated lines as written.

**Demo.** `demos/eases/show_animation.cpp`'s `show_karaoke` scene dims the narrated text
and lights each word yellow as it is spoken. Verified with a real render (Chatterbox +
Qwen aligner): the highlight tracks the speech and the SRT cues agree.

Checked on real renders (`demos/eases`, 24 fps; highlight timing measured from the
rendered frames, and which path each line took confirmed with a temporary debug print):

| Render | Aligner | Words matched | Result |
|---|---|---|---|
| `--lang en` | on | 5/5, 9/9 | real alignment |
| `--lang fr` | on | 6/6, 7/7 | real alignment |
| `--lang pl` | on | 6/6, 9/9 | real alignment (see below) |
| `--lang en --aligner-model ""` | off | - | proportional fallback |

In every render the highlighted words match the on-screen word count, line by line. The
fallback's word starts follow the character-count prediction to within a frame (e.g.
0.00/0.38/0.71/1.25/1.58 s measured vs 0.00/0.40/0.72/1.29/1.62 s predicted), and its
subtitle cues are untrimmed (they abut) as expected.

**Polish aligns in practice.** The Qwen3 Forced Aligner's documented language list does
not include Polish, but for the demo sentences it returned the right number of words with
plausible timings (also seen via `align_poc`), so `Narrator` takes the aligned path. Only
two short sentences were checked, so treat Polish alignment as working but unverified
beyond that; the proportional fallback is still there if it ever fails or miscounts.
`demos/eases/pl.huml` was added for this check.

**Text layout options** (same change set). `make_styled_text()` / `make_text()` take a
`TextLayout { align, line_spacing }`. Line spacing is now `1.2 x` the largest font size
on the line above (it was a fixed 0.5 world units), and lines can be left/center/right
aligned within the widest line. The default spacing changes how existing multi-line text
looks (single-line text is unaffected).

## Open questions for next session

- Decide whether to keep Piper as a documented fallback path or drop it from
  consideration entirely.
- Windows/macOS build validation (no Mac available yet; Windows untested).
- Thread-count sweep and Q8_0 vs F16 precision comparison for CPU RTF, if a
  CPU-only render target ends up mattering (GPU is fast enough that this may
  not be worth doing).
- File an upstream issue on `0xShug0/audio.cpp` about the Chinese/Japanese
  language gap in the Chatterbox model spec, if/when it becomes a priority.
- Polish has no forced-alignment support (Qwen3 Forced Aligner's 11 languages don't
  include `pl`) - documented gap, but in practice it aligned two demo sentences correctly
  (see "Word-level animation sync"). If a Polish line ever fails or miscounts, word sync
  falls back to proportional distribution and the subtitle cue to the whole line.
- mp4/webm subtitle muxing (`mov_text`/`webvtt`) implemented but not independently
  smoke-tested with a real render - only mkv was, this session.
- Phase 5 (mock TTS backend for ctest coverage of `Narrator`/`Narration`) is next up
  and still fully open.
