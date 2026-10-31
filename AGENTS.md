
# Voxima

## Project Summary

C++20 library to generate image and audio for creating educational videos.
Visuals are generated as vector graphics, with a text layout system supporting multiple languages.
The animation system uses C++20 coroutines to avoid complex state machines.
Narration audio is generated using TTS, again support multiple languages.
Background music and sound effects are also supported.
Multi-lingual subtitles are generated along with the narration.

## Building

Do NOT try to use cmake directly.  Build and test only through make.

On initial build (i.e. when build folder doesn't yet exist):
```
make config
```

After that, simply use:

```
make
```

Cleaning - this command will wipe the build directory, and will require "make config".
```
make clean
```

## Running tests

This will build and run tests:

```
make test
```

## Debugging

```
make debug
```


## Versioning

The version is written once, in `project( Voxima VERSION x.y.z )` in `CMakeLists.txt` (numbers only). `conanfile.py` reads
it from there, and it becomes `core/Version.h` and the plugin ABI string. Do not write it anywhere else; examples use
`find_package( Voxima REQUIRED )`. A release is a commit that bumps it, tagged `vX.Y.Z` (or `vX.Y.Z-alpha.N`).

## Examples

`examples/<name>/` are plugin projects (see `examples/CMakeLists.txt`): `make` builds them into `build/Release/plugins`
(`voxima --plugin-dir build/Release/plugins --list-scenes`). The manual includes their code with pymdownx.snippets
(`--8<-- "examples/x/x.cpp:section"`, between `// --8<-- [start:section]` and `[end:section]` markers), so code shown in
the manual is code that is built. Keep scene names unique across examples.

## Documentation

- `doc/manual/`: the developer manual (for people making videos), as linked Markdown, for people making videos with a
  source build of Voxima (installed with `make install`), not for people working on Voxima itself. `make docs` builds it
  with mkdocs-material into `doc/html/`, which is committed and installed: rebuild and commit it after changing the manual.
  Use a virtualenv with the pinned versions (`python3 -m venv .venv; .venv/bin/pip install -r doc/requirements.txt;
  make docs MKDOCS=.venv/bin/mkdocs`): other versions give different HTML, which the CI docs job rejects.
- `doc/internal/`: engineering and planning notes. Not installed.
