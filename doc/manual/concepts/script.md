# The script

The script holds the **words** of a video, one file per language, apart from the scenes that show them.
A scene asks the script for text; the script has it in every language. Adding a language means adding a file,
not changing C++.

Script files are written in [HUML](../reference/huml.md), a plain, indentation-based data format.

## A script file

`scripts/en.huml`:

```huml
meta::
  lang: "en"
  inherit: null

voices::
  narrator: "chatterbox/en-voice-zero-robert-middleton.wav"

labels::
  title: "Voxima"
  chapter-1: "Building the project"

intro: """
  A script becomes a video.
  Vector shapes<,> animated with coroutines.
  Narrated and subtitled as it plays.
"""

steps::
  - "make config"
  - "make"
```

- `key:: ` starts a section; `key: value` is a value; `- item` is a list item; `#` starts a comment.
- `"""` ... `"""` is a **multi-line** text: one entry per line. Line breaks matter: they are the points narration
  and subtitles are timed to (see [Narration](../guides/narration.md)).
- `meta` says which language the file is, and what it builds on (`inherit`, below).
- `voices` names the narrators (see [Narration](../guides/narration.md#voices)).
- `labels` is the usual place for short on-screen text: titles, captions, names.
- Everything else is yours: name sections and keys as you like.

## Reading it from a scene

```cpp
const std::string lang = script.master_language();

// a short label, with a fallback if it is missing
std::string title = script.label( lang, "title" );

// any value, by its path of keys
const huml::Value &steps = script.fetch( lang, "steps" );
for ( const auto &item: steps.as_list() )
	std::string step{ item.as_string() };

const auto &lines = script.fetch( lang, "intro" ).as_multiline();  // the lines of a multi-line text
```

A `huml::Value` is a string, number (`as_int`, `as_double`), `as_bool`, list (`as_list`), dict (`as_dict`) or
multi-line text; `is_string()`, `is_list()` and so on tell which. `master_language()` is the first
language given with `--lang`: what on-screen text is shown in. (When one video holds several languages, the
others are heard and subtitled; see [Languages](../guides/narration.md#several-languages).)

**A key that is not there never fails quietly.** `fetch` gives a *missing* value, and text made from it shows
as a magenta box with a code in it, and is listed when the render ends (see
[Missing glyphs](../guides/rendering.md#missing-glyphs)). `script.label( lang, key, fallback )` takes a fallback
for the text instead.

## Languages and regions

`--lang en` loads `en.huml` from the script directory. For a region, `--lang fr-CA` loads `fr-CA.huml`, or
`fr.huml` if there is no such file; the language is still *called* `fr-CA`, so its subtitle track and metadata
say so.

A file can build on another, so a region only has to say what differs:

```huml
# fr-CA.huml
meta::
  lang: "fr-CA"
  inherit: "fr"

labels::
  title: "Une vidéo en québécois"
```

`inherit` names another file (without `.huml`); the two are **merged**, section by section, with the inheriting
file winning. So every key has a value, and regional overrides stay small. Use `inherit: null` for a file that
builds on nothing.

## Markers: cue points in narration

Write a **marker**, `{name}`, in the narrated text where something should happen, in every language:

```huml
intro: """
  These show {a}various ease{b} animations.
"""
```
```huml
# fr.huml
intro: """
  Ceux-ci montrent {a}diverses animations{b} de lissage.
"""
```

Markers are never spoken or shown. Each language's narration gets a time for each, so a scene can say "until
`a`" or "from `a` to `b`" and follow the speech in any language, where a *word number* would not carry over (words
differ in number and length between languages). A name is letters, digits, `_` and `-`. A literal brace is a doubled one: `{{a}}`.

## Written and spoken text

Text on screen is not always what should be said. In narrated text, `<written|spoken>` shows one and says the
other:

```huml
The source lives in <src|source>, and we built <C++|see plus plus> in a weekend.
```

- `<written|spoken>`: shows `written`, says `spoken`.
- `<written>` (no bar): shows `written` and says **nothing**: a gap in the narration, for on-screen text with
  nothing to say out loud.
- Subtitles use the written text.
- `<<`, `>>` and `||` are a literal `<`, `>` and `|`. A `<` with no `>` after it is ordinary text ("a < b").

A word said the same way everywhere does not have to be written that way everywhere. The `pronounce` section of a language's
file lists them, so the narration stays readable and says them the same way each time:

```huml
pronounce::
  "cd/m²": "candelas per square meter"
  OETF: "oh ee tee eff"
  "BT.2100": "bee tee twenty one hundred"
```

It applies to what is spoken and never to what is shown or to the subtitles. It goes by whole words, so `OETF` is not found in
`OETFs`, and case matters. Where more than one could apply the longest is used. A `<written|spoken>` in the text says what it
says, whatever the list has. Like the rest of a file, the section is merged over one that `meta.inherit` names.

The `Vector shapes<,>` in the first listing is this form: the comma is shown in the subtitle but is not part of
what is said.
