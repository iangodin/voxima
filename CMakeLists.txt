
cmake_minimum_required( VERSION 3.31 )

project( canim )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

if ( WIN32 )
	add_compile_options( /utf-8 /EHsc /W4 )
else()
	add_compile_options( -Werror -Wall -Wextra -Wno-missing-field-initializers )
endif()

find_package( stb REQUIRED )
find_package( lunasvg 3.5.0 REQUIRED )
find_package( plutovg 1.3.2 REQUIRED )
find_package( harfbuzz 12.3.0 REQUIRED )
find_package( Freetype 2.14.1 REQUIRED )

add_subdirectory( mfl )

add_executable( poc main.cpp Font.cpp MFLFontFace.cpp )

target_compile_features( poc PUBLIC cxx_std_20 )
target_compile_options( poc PUBLIC -fcoroutines )
target_link_libraries( poc PUBLIC stb::stb lunasvg::lunasvg plutovg::plutovg Freetype::Freetype harfbuzz::harfbuzz mfl )

