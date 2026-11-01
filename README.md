# Voxima

Voxima is a C++20 library for making educational videos. You write the animation as ordinary C++ code, and it
renders the video.

- **Vector graphics**: shapes, charts, diagrams, geometry, and syntax-highlighted code, all as outlines that can be
  moved, scaled, faded or morphed into one another.
- **Text and math in any language**: real shaping and right-to-left layout, and formulas laid out as TeX would.
- **Animation as coroutines**: a scene is a function that reads top to bottom (`co_await fade_in( ... )`), with no
  state machines.
- **Narration**: speech made on your own machine by a text-to-speech model, in the languages of your script, with
  animation that follows the words.
- **Subtitles** for every language of the video, made along with the narration.
- **Music and sound effects**, mixed into every language's audio.

## A scene

```cpp
Scene hello( Stage &stage, Script &script )
{
	Theme &theme = *stage.theme;
	const std::string title{ script.fetch( script.master_language(), "title" ).as_string() };

	auto &words = stage.visuals.add( make_text( theme, title, FontStyle{ Intent::TITLE }.large() ) );
	move_to( words, { 0.F, 1.5F } );

	auto circle = std::make_unique<Circle>();
	circle->radius = 1.F;
	circle->color = theme.palette().blue;
	auto &dot = stage.visuals.add( std::move( circle ) );
	dot.position = { 0.F, -1.5F };

	co_await fade_in( stage, words, 0.8s );
	co_await grow_in( stage, dot, 0.6s );
	co_await indicate( stage, dot );
	co_await wait_for( stage, 1s );
}
```

Scenes are built into a **plugin**, which the `voxima` program loads. The words live in script files, one per
language, so one set of scenes serves every language.

## Getting started

Voxima is distributed as source. You need a C++20 compiler (GCC 14 or newer), CMake 3.31+, Ninja, Conan 2, Python 3,
git and ffmpeg.

```
git clone git@github.com:iangodin/voxima.git
cd voxima
make config          # fetches and builds the libraries with Conan (slow the first time)
make
python3 download_fonts.py
```

Then try the examples:

```
build/Release/voxima --plugin-dir build/Release/plugins --script-dir examples/tour/scripts --lang en --preview
```

That plays a tour of every example in a window. Narration needs two models, which are not in the repository:

```
python3 download_models.py
```

Full instructions, including how to build your own plugin, are in the [manual](https://iangodin.github.io/voxima/).

## Documentation

The developer manual is at **<https://iangodin.github.io/voxima/>**. Start with
[Installing](https://iangodin.github.io/voxima/getting-started/install/) and
[Your first video](https://iangodin.github.io/voxima/getting-started/first-video/).

Its source is in `doc/manual` (Markdown), and `make docs` builds it as a web site in `doc/html`, which is also
installed with Voxima.

The [examples](examples) are small plugin projects, one per topic. The code in the manual is taken from them, and
they are built with Voxima, so they stay working.

## Building and testing

```
make config    # the first time
make           # the Release build
make test      # builds the Debug build and runs the tests
make debug     # the Debug build
make clean     # deletes build/ (run make config again afterwards)
```

## License

Voxima is under the MIT License (see [LICENSE](LICENSE)). The libraries it uses and the speech models have their
own licenses: see [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The reference voices in `voices/` are CC0 or
public domain, with their sources in `voices/chatterbox/SOURCES.md`.
