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
| A C++20 compiler | GCC 14 or newer on Linux; MSVC 19.4 or newer on Windows |
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

Run `voxima` from the folder of the checkout, since its default places for fonts, models and voices are
relative to where it is run. (Or give them with options: `--font-dir`, `--tts-model`, `--tts-voice-dir`.)

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
| `<prefix>/share/doc/Voxima/` | The manual (as a web site, open `manual/index.html`) and the licenses |

[Your own plugin project](../guides/plugin-project.md) shows how to build against it.

## Fonts

The fonts are large and are not in the repository. Download them once, from the checkout:

```
python3 download_fonts.py
```

They go to `assets/fonts`. Voxima looks for fonts in the directory given with `--font-dir`, then in
`assets/fonts` (relative to where you run it), then in `$VOXIMA_FONTS`, then in `<prefix>/share/voxima/fonts`
(you can give the script that directory, to share the fonts between projects). Without the fonts, text cannot be
drawn.

## Speech models and voices (only for narration)

Narration is made by a local text-to-speech model, which runs on your own machine: nothing is sent anywhere. The
reference voices are in the repository, in `voices/`. The models are large (3.2 GB together) and are not; download
them from the checkout:

```
python3 download_models.py
```

They go to `models/` (which git does not track), in the folders the defaults below expect. The script skips
files you already have and resumes an interrupted download. `--all` also fetches the larger F16 versions. They
are Q8_0 GGUF conversions in the Hugging Face repository
[audio-cpp/audio.cpp-gguf](https://huggingface.co/audio-cpp/audio.cpp-gguf), under the licenses of the original
models (Chatterbox: MIT; Qwen3-ForcedAligner: Apache-2.0):

| File | Used for | Default path (relative to where you run `voxima`) | Option |
|---|---|---|---|
| [`chatterbox-q8_0.gguf`](https://huggingface.co/audio-cpp/audio.cpp-gguf/tree/main/Chatterbox-GGUF) | Speech | `models/Chatterbox-GGUF/chatterbox-q8_0.gguf` | `--tts-model` |
| [`qwen3-forced-aligner-0.6b-q8_0.gguf`](https://huggingface.co/audio-cpp/audio.cpp-gguf/tree/main/Qwen3-ForcedAligner-0.6B-GGUF) | Timing of the subtitles to the words (optional) | `models/Qwen3-ForcedAligner-0.6B-GGUF/qwen3-forced-aligner-0.6b-q8_0.gguf` | `--aligner-model` |

To download them yourself, take the two files from that repository (`Chatterbox-GGUF/` and
`Qwen3-ForcedAligner-0.6B-GGUF/`) and put them at those paths. Each narrator speaks in the voice of a short
**reference clip** (a few seconds of a real voice), named in your script's `voices` section and looked for under
`--tts-voice-dir` (default `voices`). See [Narration](../guides/narration.md#voices); the clips that come with Voxima, and their licenses, are listed there too. A video with no narration
needs none of these.

You can use a GPU for the speech with `--tts-backend cuda` (the default, `best`, picks one that works), if CUDA
was found when Voxima was configured. With `cpu`, `--tts-threads` sets how many threads it uses.

Go on to [Your first video](first-video.md).
