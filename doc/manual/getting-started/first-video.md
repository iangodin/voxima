# Your first video

This builds the small example plugin that comes with Voxima, previews it in a window, and renders it to a
video file. You need Voxima [built and installed](install.md) and the fonts.

## 1. Copy the example

```
cp -r <prefix>/share/voxima/examples/hello my-video
cd my-video
```

It holds three files:

- `hello.cpp`: one scene, written in C++.
- `scripts/en.huml`: the words, for English (a title, here).
- `CMakeLists.txt`: builds `hello.cpp` into a plugin.

## 2. Look at the scene

```cpp
--8<-- "examples/hello/hello.cpp"
```

Read it top to bottom:

- `VOXIMA_PLUGIN();` marks the file as a plugin. It says which compiler and library it was built with, so
  `voxima` can refuse a mismatch.
- `hello` is a **scene**: a coroutine that receives the `Stage` (what is drawn) and the `Script` (the words).
- The text and the circle are **visuals**, added to `stage.visuals`. Then `fade_in` and `grow_in` bring them in
  from nothing.
- `co_await` plays an animation and continues when it is over. Each line takes the time it names.
- `REGISTER_SCENE( "hello", ... )` gives the scene a name. With only one scene registered, that is what plays when you do not
  ask for another.

## 3. Build the plugin

Plugins are built with CMake, against the installed Voxima (`<voxima>` is your checkout):

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<prefix> \
    -DCMAKE_TOOLCHAIN_FILE=<voxima>/build/Release/generators/conan_toolchain.cmake
cmake --build build
```

The build type, the compiler and the standard library must be those Voxima was built with, which they are
if you use the same compiler. The Conan toolchain file gives CMake the libraries the headers include (lunasvg,
FreeType, libsndfile); see [Your own plugin project](../guides/plugin-project.md).

You now have `build/hello.so` (`build\hello.dll` on Windows).

## 4. Preview it

```
<prefix>/bin/voxima --plugin build/hello --script-dir scripts --lang en \
    --theme "Academic Classic" --preview
```

A window opens and plays the scene. **Leave it open, edit `hello.cpp`** (change a color, a time, the circle's
size), and rebuild: the scene starts again with your change, on its own. (F5 reloads the script too, so you can
change the words just as quickly.) Space pauses; Esc closes the window.

## 5. Render it

```
<prefix>/bin/voxima --plugin build/hello --script-dir scripts --lang en \
    --theme "Academic Classic" --render-dir render
```

The video is `render/final.mkv`. Choose the format with `--container` (`mp4`, `mkv`, `webm`), `--vcodec` and
`--acodec`, and the size with `--width`, `--height` and `--fps`. [Rendering](../guides/rendering.md) lists what is left in
the render directory.

## Where next

- How the pieces fit: [Overview](../concepts/overview.md), then [Animation and coroutines](../concepts/coroutines.md).
- Words in several languages, with narration: [The script](../concepts/script.md) and [Narration](../guides/narration.md).
- Drawing things: [Visuals and layout](../guides/visuals-layout.md), [Text and math](../guides/text-math.md),
  [Charts and diagrams](../guides/charts-diagrams.md).
