# Rendering and previewing

## Render a video

```
voxima --plugin build/hello --script-dir scripts --lang en --theme "Academic Classic" \
       --render-dir render --container mkv --vcodec h264 --acodec opus
```

| Option | |
|---|---|
| `--plugin` | A plugin of scenes (repeatable; the `.so` / `.dll` extension may be left out) |
| `--scene` | A scene to render; repeat to play several in order (default: the one called `movie`) |
| `--lang` | A language; repeat for several (the first is the master). See [Narration](narration.md#several-languages) |
| `--script-dir` | Where `<lang>.huml` files are |
| `--theme`, `--theme-dir`, `--palette` | Look: see [Themes](themes.md) |
| `--width`, `--height`, `--fps` | Picture size and frame rate (1280×720 at 24 fps by default) |
| `--container` | `mp4`, `mkv` or `webm` |
| `--vcodec` | `h264`, `h265`, `vp9` or `av1` |
| `--acodec` | `flac`, `wav`, `aac`, `opus` or `vorbis` |
| `--render-dir` | Where the output goes |
| `--font-dir` | Fonts (see [Installing](../getting-started/install.md#fonts)) |

Every option is in the [command line reference](../reference/command-line.md).

Not every container takes every codec; if a combination is refused, try another (`mkv` takes nearly all of them).

## What is in the render directory

- `final.<container>`: the video, with an audio and a subtitle track for every language, labeled with the language. In
  mp4 the subtitles are `mov_text`, in mkv `srt` and in webm `webvtt`.
- `subtitles.<lang>.srt` and `subtitles.<lang>.vtt`: the subtitles, for each language, to use on their own
  (YouTube takes `.srt` or `.vtt`).
- The pieces it was made from, such as each language's audio, and the `tts-cache` folder (see below).

With `--separate`, each language is in its own folder, `<render-dir>/<lang>/`, with its own `final.<container>`.

`--save-svg` and `--save-png` also keep every frame as an SVG or PNG file (a lot of files; handy for stills).

Speech is **cached**, so rendering again after a change to the animation (not the words) does not have to make the
speech again. It is kept in a `tts-cache` folder inside the render directory, or wherever `--tts-cache-dir` or
`$VOXIMA_TTS_CACHE` says. Pointing them at one folder lets renders into different directories (a draft and a final,
`--separate` runs, CI) share the speech, even at the same time. A voice is known by its path as given, so use the same
`--tts-voice-dir` for them to share.

Each line of speech that has to be made is reported as it is (`TTS en 12: 'The OETF is…' 3.1 s, took 18.2 s`), with what
it all came to at the end. `--tts-only` makes the speech and exits, with no video, to get the slow part over with
before working on the visuals (overnight, say). It does not work out word timings, which are not cached.

## Choose scenes

```
voxima --plugin build/hello --list-scenes                  # what the plugin has
voxima --plugin build/hello ... --scene intro              # one scene
voxima --plugin build/hello ... --scene intro --scene end  # several, in order
```

An unknown scene name is reported (with the list) before any model or font is loaded.

## Preview

```
voxima --plugin build/hello --script-dir scripts --lang en --theme "Academic Classic" --preview
```

`--preview` plays the video in a window with sound, instead of rendering a file (the first language only). It runs
at your display's refresh rate. Controls:

| Key | |
|---|---|
| Space | Pause and resume |
| L, K, J | Play forwards, pause, play backwards |
| Left, Right | Step one frame back or forward (paused) |
| Home, End | Go to the start or the end |
| `[` and `]` | The previous and the next scene |
| Up, Down | Skip a second forward or back |
| R | Repeat the scene being watched |
| F5 | Reload the plugin and the script |
| Esc | Close |

**Live editing:** a rebuilt plugin is loaded again by itself. The scenes start again with the new code, and the
window goes back to where it was in the scene being watched. If the new build cannot be loaded, or a scene fails,
the problem is shown in the window's title and on stderr, and the next build is tried when it comes. (What is loaded
is a copy of the plugin, so the build can write over the file.) F5 also reloads the script, so you can edit the
words too.

## Missing glyphs

A character that cannot be drawn is never left to fail quietly. Two things count:

- a character no real font in the theme has (the catch-all NotDef font at the end of every stack draws a box
  for anything, so it does not count), and
- U+FFFD, the replacement character, which is what a missing script value or badly encoded text turns into.

Each is drawn as a **magenta box with the character's code in it**, keeping the text's spacing. When the render finishes, every
one is listed with the text it was in:

```
WARNING: 2 missing glyphs (drawn as magenta boxes in the video):
  U+FFFD  in "A script value that is missing: ..."
  U+10FFFD  in "A character no font has: ..."
```

`--strict-glyphs` makes the render exit with an error if there were any, which is what you want in an automated
build. Text that is not valid UTF-8 is rejected outright.
