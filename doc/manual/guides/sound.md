# Music and sound effects

The code on this page is from the `sound` example (`examples/sound`), which is built with Voxima.

Any sound file (WAV, FLAC, Ogg, MP3) can be played into the video with `stage.audio.play`. It starts at the frame
being made, and carries on by itself, **across scenes**, until its end or until you stop it. It is mixed into
**every language's** audio track. Any number play at once.

```cpp
--8<-- "examples/sound/sound.cpp:music"
--8<-- "examples/sound/sound.cpp:fade-out"
```

`SoundOptions`:

| Option | Meaning |
|---|---|
| `volume` | In dB: 0 is as recorded, -6 about half as loud |
| `loop` | Start again at the end, until stopped or faded out |
| `fade_in` | Rise from silence to the volume over this long |
| `from` | Start this far into the sound |

`play` returns a `Sound`, to change it later: `set_volume( db, over )`, `fade_out( over )`, `stop()`. The sound plays
on whether or not you keep it, so you only need it if you will change it. A file is read once however often
it is played. `stage.audio.stop_sounds()` stops everything at once.

Volume changes are smooth ramps, so they do not click.

## Sounds made in code

`play` also takes an `AudioBuffer` of samples your program made (any rate and channel count; it is made stereo at the track's rate). The `sound` example (`examples/sound`) makes a plucked note this way:

```cpp
--8<-- "examples/sound/sound.cpp:pluck"
```

## Tying a sound to an animation

Play the sound at the moment the animation does something, in the same scene:

```cpp
--8<-- "examples/sound/sound.cpp:tie"
```

Narration is separate (it has its own voice and sync; see [Narration](narration.md)); music and effects are
mixed with it.
