# Installing

Voxima is distributed as **source code**. You build it once, then build your own scenes against that build. (A
Conan package is planned, so that this step becomes a single command.)

Building from source is also what keeps your scenes working: the scenes you write are compiled into a plugin that
the `voxima` program loads, and a plugin has to be built with the same compiler (and version), standard library,
Debug or Release configuration, and Voxima version as the program. If you build both yourself, with one compiler,
they match. A plugin that does not match is refused when it loads, with a message naming both builds, rather
than crashing.

## What you need

| | |
|---|---|
| A C++20 compiler | GCC 14 or newer on Linux; MSVC 19.4 or newer on Windows; Apple clang on macOS (21 is known to work) |
| CMake | 3.31 or newer |
| Ninja | The build uses it |
| Conan | Version 2. It downloads and builds the libraries Voxima uses (`pip install conan`) |
| Python 3 | For Conan and for downloading the fonts |
| git | To get the source |
| ffmpeg | Voxima encodes and joins video, audio and subtitles by running `ffmpeg`, so it has to be on your `PATH`. Check with `ffmpeg -version` |

Conan needs a default profile for your compiler once: `conan profile detect`. Voxima's code is C++20; set
`compiler.cppstd=20` in the profile (`~/.conan2/profiles/default`).

## Get the source

```
git clone git@github.com:iangodin/voxima.git
cd voxima
```

or download the source archive of a release from the project's GitHub page and unpack it. The clone includes the
speech engine (`extern/audio.cpp`) and the reference voices (`voices/`).

## Build it

```
make config
make
```

`make config` runs Conan to get the libraries (the first time this builds several of them, which takes a while)
and configures the Debug and Release builds. `make` builds the Release build. Then

```
build/Release/voxima --help
```

prints every option. These are the other targets:

| Command | What it does |
|---|---|
| `make debug` | Builds the Debug build, in `build/Debug` |
| `make test` | Builds Debug and runs the tests |
| `make install PREFIX=<dir>` | Installs the Release build: the program, the core library, the headers and the CMake package |
| `make clean` | Deletes `build/` (run `make config` again afterwards) |

Run `voxima` from the folder of the checkout, where it finds `assets/fonts`, `models` and `voices`. From anywhere
else it looks for them as described under [Fonts](#fonts) and
[Speech models and voices](#speech-models-and-voices-only-for-narration), or you can give them with options
(`--font-dir`, `--tts-model`, `--aligner-model`, `--tts-voice-dir`).

## Install it (to build plugins elsewhere)

Plugins of your own can be built inside the checkout, or in a project of their own that finds Voxima through
CMake. For the second, install the build somewhere:

```
make install PREFIX=$HOME/voxima-install
```

The rest of this manual calls that folder `<prefix>`. It holds

| Where | What |
|---|---|
| `<prefix>/bin/voxima` | The program: renders and previews videos |
| `<prefix>/lib/` | The core library (`libvoxima_core`) and the speech engine library the program loads |
| `<prefix>/include/voxima/` | The headers your scenes are written against |
| `<prefix>/lib/cmake/Voxima/` | The CMake package, for `find_package( Voxima )` |
| `<prefix>/share/voxima/examples/` | Example plugin projects to start from, one folder each |
| `<prefix>/share/voxima/voices/` | The reference voices for narration, with their sources and licenses |
| `<prefix>/share/voxima/download_fonts.py`, `download_models.py` | The scripts that download the fonts and the speech models |
| `<prefix>/share/doc/Voxima/` | The manual (as a web site, open `manual/index.html`) and the licenses |

[Your own plugin project](../guides/plugin-project.md) shows how to build against it.

## Fonts

The fonts are large and are not in the repository. Download them once, from the checkout:

```
python3 download_fonts.py
```

They go to `assets/fonts`, or to a directory you give the script (`--help` lists its options, and `--list` names
the fonts without downloading anything). Voxima looks for fonts in the directory given with `--font-dir`, then in
`assets/fonts` (relative to where you run it), then in `$VOXIMA_FONTS`, then in `<prefix>/share/voxima/fonts`
(you can give the script that directory, to share the fonts between projects). Without the fonts, text cannot be
drawn.

## Speech models and voices (only for narration)

Narration is made by a local text-to-speech model, which runs on your own machine: nothing is sent anywhere. The
reference voices are in the repository, in `voices/`, and are installed with Voxima. The models are large (3.2 GB
together) and are not; download them from the checkout:

```
python3 download_models.py
```

They go to `models/` (which git does not track), in the folders Voxima expects. Run from an install
(`<prefix>/share/voxima/download_models.py`), the script puts them in `<prefix>/share/voxima/models` instead, for
every project to share; or give it a directory. It skips files you already have and resumes an interrupted
download. `--all` also fetches the larger F16 versions, and `--list` says what it would fetch and where,
without downloading anything. They are Q8_0 GGUF conversions in the Hugging Face repository
[audio-cpp/audio.cpp-gguf](https://huggingface.co/audio-cpp/audio.cpp-gguf), under the licenses of the original
models (Chatterbox: MIT; Qwen3-ForcedAligner: Apache-2.0):

| File | Used for | Path in the models folder | Option |
|---|---|---|---|
| [`chatterbox-q8_0.gguf`](https://huggingface.co/audio-cpp/audio.cpp-gguf/tree/main/Chatterbox-GGUF) | Speech | `Chatterbox-GGUF/chatterbox-q8_0.gguf` | `--tts-model` |
| [`qwen3-forced-aligner-0.6b-q8_0.gguf`](https://huggingface.co/audio-cpp/audio.cpp-gguf/tree/main/Qwen3-ForcedAligner-0.6B-GGUF) | Timing of the subtitles to the words (optional) | `Qwen3-ForcedAligner-0.6B-GGUF/qwen3-forced-aligner-0.6b-q8_0.gguf` | `--aligner-model` |

Without its option, each model is looked for in `models` (relative to where you run `voxima`), then in
`$VOXIMA_MODELS`, then in `<prefix>/share/voxima/models`, and the first folder that has the file is used. To download
them yourself, take the two files from that repository (`Chatterbox-GGUF/` and
`Qwen3-ForcedAligner-0.6B-GGUF/`) and put them at those paths in one of these folders.

Each narrator speaks in the voice of a short **reference clip** (a few seconds of a real voice), named in your
script's `voices` section. The clips are looked for under `--tts-voice-dir`; without it, under the first of `voices`
(relative to where you run `voxima`), `$VOXIMA_VOICES` and `<prefix>/share/voxima/voices` that there is. See
[Narration](../guides/narration.md#voices); the clips that come with Voxima, and their licenses, are listed there
too. A video with no narration needs none of these.

You can use a GPU for the speech with `--tts-backend cuda` (the default, `best`, picks one that works), if CUDA
was found when Voxima was configured. With `cpu`, `--tts-threads` sets how many threads it uses.

Go on to [Your first video](first-video.md).
