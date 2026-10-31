# Reference voice clips

Short speaker reference clips for Chatterbox voice cloning (see `tts.md`). Each
entry names the source, license, and consent story per the table in `tts.md`.

## Licenses at a glance

Checked against the upstream sources on 2026-10-31. Every clip here is CC0 or public domain, so none needs credit
(each is credited below anyway). Clips that need credit (CC BY) are deliberately not kept here, so a video
cannot use one by accident.

| Clip | License |
|---|---|
| `en-voice-zero-*` (4 clips) | CC0 (Voice-Zero, from public-domain recordings) |
| `en-kyutai-donation-midlands.wav`, `fr-kyutai-donation-ssa150803.wav` | CC0 (`kyutai/tts-voices` `voice-donations/`) |
| `fr-`, `es-`, `de-`, `pl-librivox-*` (5 clips) | Public domain (LibriVox's policy) |

## English narrators

Five voices chosen for documentary narration (2026-10-30). Each is public
domain or CC0, so no credit is required, but every one is credited here and
should be credited in the videos that use it.

### en-voice-zero-robert-middleton.wav

- Credit: Narration voice based on a recording by Robert Middleton, narrator
  of *Duck and Cover* (1951, Archer Productions for the U.S. Federal Civil
  Defense Administration), public domain, via Voice-Zero.
- Reader: [Robert Middleton](https://www.imdb.com/name/nm0585557/), American (Ohio).
- Source: [*Duck and Cover*](https://archive.org/details/DuckandC1951) on
  Archive.org (marked public domain there); clip from
  [Voice-Zero](https://github.com/OwenTyme/voice-zero), `robert_middleton.flac`
  (CC0, cleaned for voice cloning).
- 44.1kHz mono, 11.2s. The default English narrator in `examples/narration/scripts/en.huml`.

### en-voice-zero-douglas-harlon.wav

- Credit: Narration voice based on a recording by Douglas Harlon, narrator of
  *How the Eye Functions* (1941, Karl Kurt Bosse), public domain, via Voice-Zero.
- Reader: [Douglas Harlon](https://www.imdb.com/name/nm11569469/), American.
- Source: [*How the Eye Functions*](https://archive.org/details/HowtheEy1941) on
  Archive.org (marked public domain there); clip from Voice-Zero,
  `douglas_harlon.flac` (CC0).
- 44.1kHz mono, 9.2s.

### en-voice-zero-bev-j-stevens.wav

- Credit: Narration voice based on a LibriVox recording by Bev J. Stevens
  (*Through the Brazilian Wilderness*, Theodore Roosevelt), public domain, via
  Voice-Zero.
- Reader: [Bev J. Stevens](https://librivox.org/reader/3816), American.
- Source: [LibriVox, *Through the Brazilian Wilderness*](https://librivox.org/through-the-brazilian-wilderness-by-theodore-roosevelt/),
  track 34; clip from Voice-Zero, `bev_j_stevens.flac` (CC0).
- 44.1kHz mono, 8.9s.

### en-voice-zero-emily-cripps.wav

- Credit: Narration voice based on a LibriVox recording by Emily Cripps
  (*A Select Collection of Valuable and Curious Arts and Interesting
  Experiments*), public domain, via Voice-Zero.
- Reader: [Emily Cripps](https://librivox.org/reader/18865), English.
- Source: [LibriVox, *A Select Collection of Valuable and Curious Arts…*](https://librivox.org/a-select-collection-of-valuable-and-curious-arts-and-interesting-experiments-by-unknown/),
  track 22; clip from Voice-Zero, `emily_cripps.flac` (CC0).
- 44.1kHz mono, 8.7s.

### en-kyutai-donation-midlands.wav

- Credit: Narration voice based on a recording by an anonymous volunteer
  ("Midlands"), Unmute Voice Donation Project (Kyutai), CC0.
- Source: [`kyutai/tts-voices`](https://huggingface.co/kyutai/tts-voices),
  `voice-donations/Midlands.wav`.
- License: CC0. Consent story: purpose-built donation for voice cloning,
  explicit. The volunteer gave no name, so the credit uses the clip's name.
- 32kHz mono, 10.0s.

## French, Spanish, German and Polish narrators

Chosen for documentary narration (2026-10-30). The LibriVox clips are about
10s cut from one chapter each, 24kHz mono. LibriVox releases every recording
into the public domain; the Archive.org items carry the Public Domain Mark
(except *Das Leben der Bienen*, whose item has no license field, so it relies
on LibriVox's policy). As with English, every voice is credited even though
none requires it.

### fr-kyutai-donation-ssa150803.wav

- Credit: Narration voice based on a recording by an anonymous volunteer
  ("SSA150803"), Unmute Voice Donation Project (Kyutai), CC0.
- Source: [`kyutai/tts-voices`](https://huggingface.co/kyutai/tts-voices),
  `voice-donations/SSA150803.wav`. Language detected as French.
- License: CC0. Consent story: purpose-built donation for voice cloning,
  explicit. The volunteer gave no name, so the credit uses the clip's name.
- 32kHz mono, 10.0s. The default French narrator in `examples/narration/scripts/fr.huml`.

### fr-librivox-pauline-latournerie.wav

- Credit: Narration voice based on a LibriVox recording by Pauline Latournerie
  (*Le pédagogue n'aime pas les enfants*, Henri Roorda), public domain.
- Source: [LibriVox](https://librivox.org/le-pedagogue-naime-pas-les-enfants-by-henri-roorda/),
  [Archive.org](https://archive.org/details/pedagogue_naime_pas_les_enfants_1812_librivox)
  `pedagoguenaimepasenfants_03_roorda`, from 81.1s.
- 9.8s.

### es-librivox-mario-pineda.wav

- Credit: Narration voice based on a LibriVox recording by Mario Pineda
  (*Cuentos de vacaciones*, Santiago Ramón y Cajal), public domain.
- Source: [LibriVox](https://librivox.org/cuentos-de-vacaciones-by-santiago-ramon-y-cajal/),
  [Archive.org](https://archive.org/details/cuentosdevacaciones_2408_librivox)
  `cuentosvacaciones_02_ramonycajal`, from 883.7s.
- 10.6s. The only Spanish narrator.

### de-librivox-friedrich.wav

- Credit: Narration voice based on a LibriVox recording by Friedrich
  (*Das Leben der Bienen*, Maurice Maeterlinck), public domain.
- Source: [LibriVox](https://librivox.org/das-leben-der-bienen-by-maurice-maeterlinck/),
  [Archive.org](https://archive.org/details/daslebenderbienen_2404_librivox)
  `lebenderbienen_03_maeterlinck`, from 513.9s. The reader is credited on
  LibriVox by first name only.
- 9.7s. The default German narrator in `examples/narration/scripts/de.huml`.

### pl-librivox-gosia-leksandrowna.wav

- Credit: Narration voice based on a LibriVox recording by Gosia Leksandrówna
  (*Ballady i romanse*, Adam Mickiewicz), public domain.
- Source: [LibriVox](https://librivox.org/ballady-i-romanse-by-adam-mickiewicz/),
  [Archive.org](https://archive.org/details/balladyiromanse_1703_librivox)
  `balladyiromanse_03_mickiewicz`, from 208.1s.
- 10.4s. The default Polish narrator in `examples/narration/scripts/pl.huml`.

### pl-librivox-piotr-nater.wav

- Credit: Narration voice based on a LibriVox recording by Piotr Nater
  (*Wilk, psy i ludzie*, Adolf Dygasiński), public domain.
- Source: [LibriVox](https://librivox.org/wilk-psy-i-ludzie-by-adolf-dygasinski/),
  [Archive.org](https://archive.org/details/wilk_psy_i_ludzie_1812_librivox)
  `wilkpsyiludzie_03_dygasinski`, from 372.9s.
- 10.0s.

## Removed clips

`en-alba-mackenna-casual.wav` (CC BY 4.0, `kyutai/tts-voices` `alba-mackenna/`) and
`es-cml-tts-3719_1378_000081.wav` (CC BY 4.0, CML-TTS) were removed on 2026-10-31, because CC BY needs credit and
a clip could be used without it. The earlier French, German and Polish clips (CML-TTS speakers 10087 and 11295,
and Thorsten Müller) were removed on 2026-10-30.

## Chinese (zh) — not sourced

No reference voice was sourced for Chinese. See `tts.md` "Reference voices" —
confirmed by direct test (2026-09-23): `tts_poc ... zh` fails outright with
`Error: run: unsupported Chatterbox language: zh`, a hard rejection from
audio.cpp's own Chatterbox backend (matches `model_specs/chatterbox.json`,
whose `languages` list omits `zh`), not silently-wrong output. Chinese is
not functional through this integration regardless of voice licensing, so
sourcing a voice was not attempted.
