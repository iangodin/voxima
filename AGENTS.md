
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

