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

Open items to verify in a validation spike:
- **Chinese language coverage is ambiguous.** One doc surface listed 19 supported
  languages without `zh` explicitly (ar, da, de, el, en, es, fi, fr, hi, it, ko, ms,
  nl, no, pl, pt, sv, sw, tr); another mentioned a `cangjie_mapping.json` (Chinese g2p
  asset) bundled in. Resemble's own model card lists Chinese for Multilingual v3.
  Need to test directly rather than trust either doc summary.
- **No documented determinism/seed controls** in audio.cpp. Mitigated in practice by
  our existing cache (`TTSClient.cpp`), which stores exact synthesized bytes per
  `(text, speaker, language, rate)` hash and reuses them forever — so *within a
  project*, output is already bit-reproducible once cached. What actually matters is
  **speaker-identity consistency** across different lines, which comes from using a
  fixed reference clip per named voice, not from model-level sampling determinism.
- Confirm the GGUF conversion doesn't add license terms beyond audio.cpp's Apache-2.0
  and Chatterbox's own MIT weights.
- Measure CPU-only real-time-factor (audio.cpp is CUDA-optimized; CPU fallback exists
  but is described as "substantially slower"). Canim has no GPU dependency today.
  First-time render cost matters even though the cache absorbs repeat renders.

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
| en | [`kyutai/tts-voices`](https://huggingface.co/kyutai/tts-voices) — Unmute Voice Donation Project subset | CC0 | Purpose-built donation, explicit | 228 verified voices, ~10s clips, 48kHz WAV. Same HF collection also bundles VCTK (CC-BY 4.0, 109 speakers, studio 48kHz) as a fallback/alternate. |
| fr | `kyutai/tts-voices` — Voice Donation Project subset | CC0 | Purpose-built donation, explicit | Same collection also bundles CML-TTS/French (CC-BY 4.0). |
| de | [Thorsten-Voice](https://github.com/thorstenMueller/Thorsten-Voice) | **CC0** | Purpose-built, explicit — Thorsten Müller recorded and released it specifically for open TTS use | Best consent story of anything found. 23+ hours, 22,668 phrases, 44kHz, single fixed speaker. Separate emotional-variant datasets exist too (neutral/amused/sleepy/whispering/etc. on OpenSLR). Good candidate for a reference-clip-length sensitivity test (5s vs 20s clip, same speaker) before locking in clip lengths elsewhere. |
| pl | [CML-TTS](https://arxiv.org/html/2306.10097) | CC-BY 4.0 | Repurposed (LibriVox-style audiobook narration) | Also covers es, fr (redundant with Kyutai), de (redundant with Thorsten), it, pt, nl. |
| es | CML-TTS | CC-BY 4.0 | Repurposed | See above. |
| zh | **none found** | — | — | Open gap. `AISHELL-3` (218 speakers) and `Aidatatang_200zh` (600 speakers) exist but are licensed "free for academic/research use," not clearly commercial-safe. Need to either vet one of those licenses carefully, use whatever default/demo reference Chatterbox ships (if any), or source/record a consenting Mandarin speaker ourselves. |

Explicitly avoid: Expresso and EARS from the Kyutai collection — **CC-BY-NC**,
non-commercial only.

Plan: pick one reference clip per language/voice, check the clip + a short license/
source note into the repo (or a documented fetch step), replacing the current
`voices` config in `Script` (currently a plain edge-tts `ShortName` string) with a
reference-clip path per named voice.

## Rough phase breakdown (not yet scheduled)

1. **Validation spike** — build audio.cpp, load the Chatterbox Multilingual GGUF,
   manually synthesize en/fr/es/pl/de/zh samples via CLI. Settle the Chinese coverage
   question. Measure CPU-only RTF. Test speaker-identity consistency across lines from
   one reference clip. Test the Thorsten-Voice clip-length sensitivity question.
   Confirm license chain (audio.cpp Apache-2.0 + Chatterbox MIT weights, no surprises
   in the GGUF conversion).
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

## Open questions for next session

- Resolve Chinese reference voice sourcing (or accept the gap for now).
- Decide whether to keep Piper as a documented fallback path or drop it from
  consideration entirely.
- Windows/macOS build validation (no Mac available yet; Windows untested).
