# A plugin of scenes. Built by itself, as in hello/CMakeLists.txt; inside Voxima's own build, make builds it as
# build/Release/plugins/sound.so.
cmake_minimum_required( VERSION 3.31 )

project( SoundVoxima CXX )

if ( NOT TARGET Voxima::core )
	find_package( Voxima REQUIRED )
endif()

voxima_add_plugin( sound sound.cpp )
