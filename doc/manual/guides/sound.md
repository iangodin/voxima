# Music and sound effects

Any sound file (WAV, FLAC, Ogg, MP3) can be played into the video with `stage.audio.play`. It starts at the frame
being made, and carries on by itself, **across scenes**, until its end or until you stop it. It is mixed into
**every language's** audio track. Any number play at once.

```cpp
Sound music = stage.audio.play( "assets/music/theme.mp3", { .volume = -18, .loop = true, .fade_in = 2s } );
stage.audio.play( "assets/sfx/pop.wav", { .volume = -6 } );
...
music.fade_out( 3s );
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

`play` also takes an `AudioBuffer` of samples your program made (any rate and channel count; it is made stereo at the track's rate). The eases demo (`demos/eases/show_animation.cpp`) makes a plucked note this way:

```cpp
AudioBuffer note = make_my_note( 440.F );                    // your own code fills the samples
stage.audio.play( note, { .volume = -14.0 } );
```

## Tying a sound to an animation

Play the sound at the moment the animation does something, in the same scene:

```cpp
stage.audio.play( "pop.wav" );
co_await grow_in( stage, dot, 0.4s );
```

Narration is separate (it has its own voice and sync; see [Narration](narration.md)); music and effects are
mixed with it.
