# A plugin of scenes, built against an installed Voxima. In Voxima:
#
#     make install PREFIX=<dir>
#
# then here, with the same compiler and build type (see src/core/Plugin.h), and the libraries Voxima's headers
# use (the Conan toolchain Voxima was built with has them):
#
#     cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<dir> \
#         -DCMAKE_TOOLCHAIN_FILE=<voxima>/build/Release/generators/conan_toolchain.cmake
#     cmake --build build
#     <dir>/bin/voxima --plugin build/hello --script-dir scripts --lang en --theme "Academic Classic" --preview
cmake_minimum_required( VERSION 3.31 )

project( HelloVoxima CXX )

find_package( Voxima 0.1.0 REQUIRED )

voxima_add_plugin( hello hello.cpp )
