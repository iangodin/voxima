# Reference voice clips

Short speaker reference clips for Chatterbox voice cloning (see `tts.md`). Each
entry names the source, license, and consent story per the table in `tts.md`.

## en-alba-mackenna-casual.wav

- Source: [`kyutai/tts-voices`](https://huggingface.co/kyutai/tts-voices),
  `alba-mackenna/casual.wav`.
- License: **CC BY 4.0** (corrected 2026-09-23 — originally logged here as CC0,
  which was wrong; the repo's own README states the `alba-mackenna/` collection
  is "Released under the CC BY 4.0 licence", not CC0 like `voice-donations/`).
  Attribution: Alba MacKenna, via kyutai/tts-voices.
- Consent story: purpose-built voice-acting for the dataset, but under CC BY
  4.0 rather than the strongest (CC0) consent story — downgraded from "purpose-
  built donation, explicit" to "purpose-built, attribution required" in tts.md.
- 48kHz mono, ~10s. Used as the English proof-of-concept reference clip for
  `tools/tts_poc.cpp`.

## fr-cml-tts-10087_11650_000028-0002.wav

- Source: [`kyutai/tts-voices`](https://huggingface.co/kyutai/tts-voices),
  `cml-tts/fr/10087_11650_000028-0002.wav` (French subset of the CML-TTS
  dataset, bundled into this collection).
- License: CC BY 4.0.
- Consent story: repurposed (CML-TTS is audiobook-style narration licensed
  openly for research/dataset use, not a purpose-built voice donation).
- 24kHz mono, 7.24s.

## de-thorsten-voice-short.wav

- Source: [`Thorsten-Voice/TV-44kHz-Full`](https://huggingface.co/datasets/Thorsten-Voice/TV-44kHz-Full)
  (canonical HF mirror of [thorstenMueller/Thorsten-Voice](https://github.com/thorstenMueller/Thorsten-Voice)),
  subset `TV-2021.02-Neutral`, resampled 44.1kHz -> 24kHz mono.
- License: **CC0** (confirmed against both the HF dataset card, tagged
  `cc0-1.0`, and the author's own GitHub README: "released ALL recordings...
  Obviously again in CC0 license!"). Note: a separate third-party HF mirror,
  `Thorsten-Voice/TV-24kHz-Neutral`, mislabels the same recordings as
  CC-BY-4.0 — use `TV-44kHz-Full` as the source of truth, not that repo.
- Consent story: purpose-built, explicit — Thorsten Müller recorded and
  released this specifically for open TTS use. Speaker: Thorsten Müller.
- 24kHz mono, 5.48s ("Der Stadtführer trug einen auffälligen Anorak...").
- A second, longer clip from the same speaker/subset (13.29s, "Ab
  siebzehnhundertsiebenundsechzig war er in Berlin...") was used for the
  clip-length sensitivity test in tts.md but not checked in here — regenerate
  from the same HF dataset shard (`TV-2021.02-Neutral/train-00000-of-00015.parquet`,
  longest-duration row) if needed again.

## es-cml-tts-3719_1378_000081.wav

- Source: [`freds0/cml_tts_dataset_spanish`](https://huggingface.co/datasets/freds0/cml_tts_dataset_spanish)
  (HF parquet mirror of the [CML-TTS](https://arxiv.org/html/2306.10097)
  Spanish subset), `dev/audio/3719/1378/3719_1378_000081.flac`, converted to
  24kHz mono WAV.
- License: CC BY 4.0 (per CML-TTS's own dataset license; the HF mirror's own
  card doesn't carry a license tag, so this is inherited from upstream
  CML-TTS/OpenSLR-146, not re-verified against the mirror itself).
- Consent story: repurposed (LibriVox-style audiobook narration).
- 24kHz mono, 10.91s.

## pl-cml-tts-11295_11059_000062.wav

- Source: [`freds0/cml_tts_dataset_polish`](https://huggingface.co/datasets/freds0/cml_tts_dataset_polish)
  (HF parquet mirror of the CML-TTS Polish subset),
  `dev/audio/11295/11059/11295_11059_000062.flac`, converted to 24kHz mono WAV.
- License: CC BY 4.0 (same basis/caveat as the Spanish entry above).
- Consent story: repurposed (LibriVox-style audiobook narration).
- 24kHz mono, 10.88s.

## Chinese (zh) — not sourced

No reference voice was sourced for Chinese. See `tts.md` "Reference voices" —
confirmed by direct test (2026-09-23): `tts_poc ... zh` fails outright with
`Error: run: unsupported Chatterbox language: zh`, a hard rejection from
audio.cpp's own Chatterbox backend (matches `model_specs/chatterbox.json`,
whose `languages` list omits `zh`), not silently-wrong output. Chinese is
not functional through this integration regardless of voice licensing, so
sourcing a voice was not attempted.
