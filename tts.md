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
a separate **forced-alignment model** (Meta MMS-300M, CTC-based, 1000+ languages) that
takes our known script text plus the generated audio and produces acoustically-grounded
word-level timestamps as a second pass.

Implication: subtitle sync becomes a two-step pipeline per line (synthesize, then
align) instead of one. More robust in principle (grounded in the actual audio, not the
model's internal timing estimate), but it's an extra inference call and a new failure
mode to handle per line. Needs to be scoped as part of the Subtitles task
(tasks.md `## Subtitles`) once this lands.

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
| en | [`kyutai/tts-voices`](https://huggingface.co/kyutai/tts-voices), `alba-mackenna/casual.wav` | **CC BY 4.0** (corrected 2026-09-23 — see below) | Purpose-built voice acting, attribution required | Checked in: `voices/chatterbox/en-alba-mackenna-casual.wav`, 48kHz, ~10s. |
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
2. **Backend swap** — stand up audio.cpp (per the integration decision above) as the
   new local TTS backend, replacing `edge_tts_server.py`. Retire `protocol.capnp` and
   the Python bridge if going with the C ABI. Replace `voices` config with reference-
   clip paths.
3. **Carry over fixes identified along the way**:
   - Fix the `Narrator::speech()` duration bug.
   - Add retry/backoff and a fail-fast reachability/availability check before a render
     starts (today nothing auto-launches or health-checks the TTS backend).
   - Version the cache key (include a model/backend identifier) so a future model
     upgrade doesn't silently serve stale cached audio.
4. **Subtitle sync** — wire in the forced-alignment pass, feeding the existing
   Subtitles task (tasks.md).
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

## Open questions for next session

- **Chinese gap: accept for now, or pursue upstream?** Needs your decision
  (see above) — this is the one item from phase 1 that isn't a clean
  "resolved," it's a resolved *diagnosis* with an open *decision*.
- Decide whether to keep Piper as a documented fallback path or drop it from
  consideration entirely.
- Windows/macOS build validation (no Mac available yet; Windows untested).
- Thread-count sweep and Q8_0 vs F16 precision comparison for CPU RTF, if a
  CPU-only render target ends up mattering (GPU is fast enough that this may
  not be worth doing).
