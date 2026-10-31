# Your own plugin project

Scenes live in a **plugin**: a shared library (`.so` on Linux, `.dll` on Windows) that `voxima` loads with
`--plugin`. A plugin project is a small CMake project built against your Voxima build, installed with
`make install PREFIX=<prefix>` (see [Installing](../getting-started/install.md#install-it-to-build-plugins-elsewhere)).
A starting point is in `<prefix>/share/voxima/examples` (or `examples/` in the checkout), one folder per example.

## The CMake project

```cmake
cmake_minimum_required( VERSION 3.31 )
project( MyVideo CXX )

find_package( Voxima REQUIRED )

voxima_add_plugin( myvideo scenes.cpp more_scenes.cpp )
```

`voxima_add_plugin( name sources... )` makes a module called `name`, links it to Voxima's core, turns on C++20 and
coroutines, and names it `name.so` or `name.dll`. Exactly one of its source files must contain `VOXIMA_PLUGIN();`.

`find_package( Voxima )` finds the Voxima you installed, and the plugin is built against its headers. A plugin is only
good for that Voxima: one loaded by a `voxima` of another version is refused (see below). To insist on a version,
write it (`find_package( Voxima 0.1.0 REQUIRED )`); only that exact version matches.

Configure with the install folder as the prefix path, and the Conan toolchain of the Voxima build (it
finds the libraries the headers use, see below):

```
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<prefix> \
    -DCMAKE_TOOLCHAIN_FILE=<voxima>/build/Release/generators/conan_toolchain.cmake
cmake --build build
```

## Building just as the program was

Scenes share the program's strings, functions and coroutines, so a plugin must be built with:

- the **same compiler and version**,
- the same **standard library**,
- the same **Debug or Release** configuration (`make install` installs the Release build),
- the same **Voxima version**.

Each plugin records what it was built with. One that does not match is refused when loaded, naming both, rather
than crashing in some distant way. On Linux a plugin should be built with the system's default libstdc++ ABI.

## Libraries the headers use

Voxima's headers include lunasvg, FreeType and libsndfile, so CMake has to be able to find them.
The Conan toolchain file of the Voxima build, as above, has them: they are the very libraries Voxima was built
with, which is also what keeps them compatible.

## Several plugins

`--plugin` can be given more than once, and scene names from all of them are one list. `--plugin-dir <folder>`
loads every plugin (`.so` or `.dll`) in a folder, in name order, so a folder of plugins is one command:

```
voxima --plugin-dir build/plugins --list-scenes
```

A file in the folder that cannot be loaded (built with another compiler, or not a plugin) is skipped with a
warning, rather than stopping the rest; with `--plugin`, it is an error. Keep plugins in a folder of their own,
since the core library is a `.so` too. A scene in one plugin can
`play` a scene from another by name. If two plugins register the same name, the one loaded later takes it, so keep names distinct.

## Working fast

Use `--preview` and **keep the window open while you build**: the plugin is loaded again when it is rebuilt, and
the scene you were watching starts again with your change. See [Rendering](rendering.md#preview).

## Debugging

Build with `-DCMAKE_BUILD_TYPE=Debug` against a **Debug** build of Voxima (`make debug` makes it; install it
with `cmake --install build/Debug --prefix <dir> --component Voxima`, with its own `<dir>`). A scene that throws shows the message in the
preview window's title and on stderr.

## If the plugin is refused

A plugin built with a different compiler, standard library, configuration or Voxima version is refused when it is
loaded. The message names what the plugin was built with and what the program was built with. Rebuild the plugin with
the compiler and configuration Voxima was built with.
