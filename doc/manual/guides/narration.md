# Narration, subtitles and languages

A scene can be **narrated**: a text-to-speech model reads lines from the [script](../concepts/script.md) in a
chosen voice, in every language of the video, and the animation follows the speech. Subtitles for every language
are made along with it.

The code on this page is from the `narration` example (`examples/narration`), which is built with Voxima.

## Voices

A narrator is a name in the script's `voices` section, pointing at a short **reference clip**; the speech is made
in that voice.

```huml
voices::
  narrator: "chatterbox/en-voice-zero-robert-middleton.wav"
```

The path is relative to `--tts-voice-dir` (without it: `voices`, `$VOXIMA_VOICES`, or the installed
`<prefix>/share/voxima/voices`, see [Speech models and voices](../getting-started/install.md#speech-models-and-voices-only-for-narration)).
A project with its own clips can keep them in a `voices` folder, but then the installed ones are not looked at: copy
any of them it also uses. Give each language its own clip, in a voice of that
language, in that language's file. Use clips you have the right to use, and credit them as their licenses ask
for (a voice's source and license should be kept next to it).

The clips in Voxima's `voices/chatterbox/` (installed in `<prefix>/share/voxima/voices/chatterbox/`) are all CC0 or public domain (LibriVox recordings, the Kyutai voice
donations and Voice-Zero), so no credit is needed, though crediting the readers is kind. The source of each is
in `voices/chatterbox/SOURCES.md`.

Chatterbox speaks many languages (English, French, German, Spanish and Polish among them). Chinese and
Japanese text can be shown but **cannot be narrated**. Subtitle timing (the optional aligner model) is not
available for every language either; a language without it still gets subtitles, timed by distributing the
words along each line.

## Speaking in a scene

```cpp
--8<-- "examples/narration/narration.cpp:intro"
```

- `setup( name, script, stage.audio )` finds the voice for **every language** in use.
- `speech( keys... )` fetches the multi-line text under those keys in each language, makes the audio for all of
  them, and returns how long the longest takes. The speech is only *made* here; it is *heard* when `play()`
  runs.
- `play()` is a scene: it runs for as long as the narration does. Run it **alongside** the animation with `choreograph`.

You can call `speech()` again after the previous one is finished, to say something else later in the same scene.

## Following the speech

Line breaks and markers in the script are the places to sync to, and they **line up across languages**.

**Lines.** Each line of the multi-line text starts a sync point:

```cpp
--8<-- "examples/narration/narration.cpp:until-line"
```

This is the robust choice: it is correct in every language and even when the shown text and the spoken text
differ in length (`<written|spoken>`).

**Markers.** For finer points inside a line, write `{name}` in every language's text, then:

```cpp
--8<-- "examples/narration/narration.cpp:markers"
```

`track_markers` calls your function every frame with how far through the words between the markers the
narration has got (and exactly 1 at the end): a morph tied to it lands on the words.

**Words.** `until_word( i )`, `until_word_end( i )` and `track_words( first, last, fn )` use word numbers of the
**first language** only: words differ in number and length between languages, so use lines and markers when the
video is in more than one.

`until_time( seconds )` and `track_time( from, to, fn )` use seconds since the batch started.

!!! tip "Sync by line or marker"
    Narration in another language takes a different time. Animations that wait on `until_line` and
    `until_marker` follow whatever each language takes; fixed `wait_for` times do not.

## Subtitles

Subtitles are made for every narrated line, in every language, from the written text (`<written|spoken>` shows
the written side). They are written as `subtitles.<lang>.srt` and `.vtt` in the render directory, and put in the
video as one subtitle track per language. A subtitle that you want on the screen *itself* (a caption in the
picture) is an ordinary visual: show the line when `until_line( i )` is reached.

## Word effects

Light up each word as it is said (karaoke), pop it, underline it:

```cpp
--8<-- "examples/narration/narration.cpp:animate-words"
```

Ready-made effects: `reveal()` (invisible until said, then fades in), `color( before, during, after )`,
`pop( scale )`, `underline( color )`; `all( { ... } )` combines them. The colors are blended, so give the colors
the word is *seen* in: for the text color use `stage.foreground`, not white. For several lines, pass
`first_word` (the number of the line's first word in the narration).

You can write your own effect: it is a coroutine receiving a `WordContext` (the word, its times, the clock):

```cpp
--8<-- "examples/narration/narration.cpp:wiggle"
```

`narration.on_words( text, callback )` is the simpler form: call a function as each word starts.

## Several languages

```
voxima ... --lang en --lang fr              # one video: an audio and a subtitle track for each language
voxima ... --lang en --lang fr --separate   # one video per language, in <render-dir>/en/ and <render-dir>/fr/
```

- **Together** (the default): one video, timed to the first language (the *master*), with a track for each
  language. The languages are **kept in step**: narration is divided at its markers and at each line start, and every
  segment is given the time the slowest language takes, so a marker falls at the same moment in every
  language's audio. The faster languages get silence at the end of a segment: nobody is cut off. `--max-speedup 1.15` lets
  the others be speeded up by up to 15% (needs ffmpeg) so there is less silence; the master is never speeded up.
  A language whose markers or lines do not match the master's cannot be kept in step: it is warned about and left as it is.
- **`--separate`:** each language is a complete render of its own, with its text on screen and its animation timed
  to its own narration, so scenes must follow the narration (`until_line`, `until_marker`) rather than fixed times.
  The models are loaded once for all of them.
