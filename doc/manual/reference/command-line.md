# Command line

Every option of `voxima`, as printed by `voxima --help`. Scenes come from plugins (see [Scenes and movies](../concepts/scenes.md)).

```
Voxima


build/Release/voxima [OPTIONS]


OPTIONS:
  -h,     --help              Print this help message and exit
          --version           Print the version and exit
  -c,     --container TEXT    Container (mp4, mkv, webm)
  -v,     --vcodec TEXT       Video codec (h264, h265, vp9 or av1)
  -a,     --acodec TEXT       Audio codec (flac, wav, aac, opus, vorbis)
          --font-dir TEXT     Directory containing fonts (without it: assets/fonts,
                              $VOXIMA_FONTS, or the installed share/voxima/fonts)
          --render-dir TEXT   Directory for render to
          --save-svg          Save SVG sequence
          --save-png          Save PNG sequence
          --preview           Play in a window with sound instead of rendering a video (first
                              language only)
          --script-dir TEXT   Directory containing scripts
          --theme TEXT        Theme to use: a built-in one by name, or a theme file (a path, or
                              <name>.huml in a --theme-dir). Without it, theme.huml in the
                              script directory if there is one, or else Modern Technical
          --theme-dir TEXT ... 
                              Directory to look in for theme files, and for the themes they
                              inherit (repeatable; the script directory is always searched)
          --palette TEXT      Colors to use, apart from the theme's fonts: academic, blueprint,
                              catppuccin-latte, catppuccin-mocha, github-dark, github-light,
                              one-dark, paper, solarized-light. Without it, the theme's own on
                              the usual black
          --width INT         Camera width
          --height INT        Camera height
          --fps INT           Camera frame rate (24 by default; with --preview, the display's
                              refresh rate)
          --lang TEXT ...     Language to use (first one is master, multiple allowed)
          --tts-model TEXT    Path to the Chatterbox GGUF model (without it: looked for in
                              models, $VOXIMA_MODELS, and the installed share/voxima/models)
          --tts-voice-dir TEXT 
                              Base directory for narrator reference-clip paths (without it:
                              voices, $VOXIMA_VOICES, or the installed share/voxima/voices)
          --tts-backend TEXT  TTS backend: best, cpu or cuda
          --tts-threads INT   TTS CPU thread count
          --aligner-model TEXT 
                              Path to the Qwen3 Forced Aligner GGUF model, for subtitle timing
                              (optional; without it: looked for as --tts-model is)
          --scene TEXT ...    Scene to render; repeat it to play several in order (default:
                              "movie")
          --max-speedup FLOAT How much the other languages may be speeded up to keep in step
                              with the first, in one video (1.0: not at all, they are padded
                              with silence; 1.15: up to 15%; needs ffmpeg)
          --separate          One video per language, each timed to its own narration, in
                              <render-dir>/<lang>/ (instead of one video with a track for each
                              language)
          --strict-glyphs     Exit with an error if any text had a missing glyph (they always
                              show as magenta boxes and are listed at the end)
          --list-scenes       List the scenes that can be rendered, and exit
          --plugin TEXT ...   A plugin of scenes (a .dll or .so; the extension may be left out)
                              to load (repeatable); with --preview, it is loaded again when it
                              is rebuilt
          --plugin-dir TEXT ... 
                              A directory of plugins: every plugin in it (a .dll or .so) is
                              loaded, in name order (repeatable). One that cannot be loaded is
                              skipped, with a warning. With --preview, they are loaded again
                              when rebuilt
```
