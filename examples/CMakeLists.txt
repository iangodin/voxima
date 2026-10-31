# All the examples. Each is a plugin project of its own, which can be copied out and built by itself (see its
# CMakeLists.txt); here they are built together, so that they are kept working. Inside Voxima's build they go to
# <build>/plugins, a directory of nothing else, which voxima --plugin-dir can load whole:
#
#     build/Release/voxima --plugin-dir build/Release/plugins --list-scenes
#
# Built by itself, this is the same as building each one.
cmake_minimum_required( VERSION 3.31 )

if ( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	project( VoximaExamples CXX )
	find_package( Voxima 0.1.0 REQUIRED )
else()
	set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins )
	set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plugins ) # (DLLs, on Windows)
endif()

add_subdirectory( hello )
add_subdirectory( coroutines )
add_subdirectory( layout )
add_subdirectory( text-math )
add_subdirectory( animation )
add_subdirectory( charts )
add_subdirectory( morphing )
add_subdirectory( narration )
add_subdirectory( sound )
