
# Sonimation

## Project Summary

C++20 library to generate image and audio for creating educational videos.
Visuals are generated as vector graphics, with a text layout system supporting multiple languages.
The animation system uses C++20 coroutines to avoid complex state machines.
Narration audio is generated using TTS, again support multiple languages.
Background music and sound effects are also supported.
Multi-lingual subtitles are generated along with the narration.

## Building

On initial build (when build folder doesn't yet exist):
```
make config
```

After that, simply use:

```
make
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

