# A plugin of scenes. Standalone, against an installed Voxima (make install PREFIX=<dir>), with the same compiler
# and build type (see src/core/Plugin.h), and the libraries Voxima's headers use (the Conan toolchain Voxima was
# built with has them):
#
#     cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=<dir> \
#         -DCMAKE_TOOLCHAIN_FILE=<voxima>/build/Release/generators/conan_toolchain.cmake
#     cmake --build build
#     <dir>/bin/voxima --plugin build/hello --script-dir scripts --lang en --theme "Academic Classic" --preview
#
# (Inside Voxima's own build, make builds it as build/Release/plugins/hello.so.)
cmake_minimum_required( VERSION 3.31 )

project( HelloVoxima CXX )

if ( NOT TARGET Voxima::core )
	find_package( Voxima REQUIRED )
endif()

voxima_add_plugin( hello hello.cpp )
