# A plugin of scenes, built against an installed Sonimation. In Sonimation:
#
#     make install PREFIX=<dir>
#
# then here, with the same compiler and build type (see src/core/Plugin.h), and the libraries Sonimation's headers
# use (the Conan toolchain Sonimation was built with has them):
#
#     cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<dir> \
#         -DCMAKE_TOOLCHAIN_FILE=<sonimation>/build/Release/generators/conan_toolchain.cmake
#     cmake --build build
#     <dir>/bin/sonimation --plugin build/hello --script-dir scripts --lang en --theme "Academic Classic" --preview
cmake_minimum_required( VERSION 3.31 )

project( HelloSonimation CXX )

find_package( Sonimation 0.1.0 REQUIRED )

sonimation_add_plugin( hello hello.cpp )
