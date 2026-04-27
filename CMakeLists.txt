
cmake_minimum_required( VERSION 3.31 )

project( canim )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )
set( CMAKE_CXX_STANDARD 20 )
set( CMAKE_CXX_STANDARD_REQUIRED ON )

if ( WIN32 )
	add_compile_options( /utf-8 /EHsc /W4 )
else()
	add_compile_options( -Werror -Wall -Wextra -Wno-missing-field-initializers )
endif()

find_package( stb REQUIRED )
find_package( lunasvg REQUIRED )

add_executable( poc main.cpp Font.cpp )

target_compile_options( poc PUBLIC -fcoroutines )
target_link_libraries( poc PUBLIC stb::stb lunasvg::lunasvg )

