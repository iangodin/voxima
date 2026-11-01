# Script keys

The keys Voxima itself reads in a script file. Everything else in a script is yours; your scenes read it with
`script.fetch( lang, keys... )` (see [The script](../concepts/script.md)).

| Key | Type | Meaning |
|---|---|---|
| `meta.lang` | text | The language the file is |
| `meta.inherit` | text or `null` | Another script file (without `.huml`) this one is merged over; `null` for none |
| `voices.<name>` | text | The reference clip for the narrator `<name>`, relative to the voice directory (see `--tts-voice-dir`) |
| `labels.<key>` | text | Short on-screen text, by convention (`script.label( lang, key )`) |
| `chapters.<scene>` | text | The title of a scene that is a chapter of the video (see [Chapters](../guides/rendering.md#chapters)) |
| `pronounce.<written>` | text | How a word is said wherever it appears in narrated text (see [the script](../concepts/script.md#written-and-spoken-text)) |

## Text syntax in narrated and shown text

| Written | Meaning |
|---|---|
| `{name}` | A marker (cue point), in every language; never spoken or shown |
| `{{` | A literal `{` |
| `<written\|spoken>` | Shows `written`, says `spoken` |
| `<written>` | Shows `written`, says nothing |
| `<<`, `>>`, `\|\|` | A literal `<`, `>`, `\|` |

A line break in a multi-line text is a sync point (`narration.until_line( i )`) and starts a new subtitle cue.

## Files

| File | Used for |
|---|---|
| `<script-dir>/<lang>.huml` | The words for a language (`--lang <lang>`). `fr-CA` falls back to `fr` if there is no `fr-CA.huml` |
| `<script-dir>/theme.huml` | A theme used when `--theme` is not given (see [Themes](../guides/themes.md#theme-files)) |
