# A plugin of scenes. Built by itself, as in hello/CMakeLists.txt; inside Voxima's own build, make builds it as
# build/Release/plugins/charts.so.
cmake_minimum_required( VERSION 3.31 )

project( ChartsVoxima CXX )

if ( NOT TARGET Voxima::core )
	find_package( Voxima 0.1.0 REQUIRED )
endif()

voxima_add_plugin( charts charts.cpp )
