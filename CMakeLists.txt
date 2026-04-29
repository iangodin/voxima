
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

add_library( visuals STATIC
	Font.cpp
	MFLFontFace.cpp
	Freetype.cpp
	freetype_error.cpp
	Theme.cpp
	FontStack.cpp
	NewComputerModern.cpp
	CreateText.cpp
)

target_compile_features( visuals PUBLIC cxx_std_20 )
target_compile_options( visuals PUBLIC -fcoroutines )
target_link_libraries( visuals PUBLIC stb::stb lunasvg::lunasvg plutovg::plutovg Freetype::Freetype harfbuzz::harfbuzz mfl )

add_executable( test main.cpp )
target_compile_features( test PUBLIC cxx_std_20 )
target_compile_options( test PUBLIC -fcoroutines )
target_link_libraries( test PUBLIC visuals )

add_executable( math_test math_test.cpp )
target_compile_features( math_test PUBLIC cxx_std_20 )
target_compile_options( math_test PUBLIC -fcoroutines )
target_link_libraries( math_test PUBLIC visuals )

add_executable( test_font test_font.cpp )
target_compile_features( test_font PUBLIC cxx_std_20 )
target_compile_options( test_font PUBLIC -fcoroutines )
target_link_libraries( test_font PUBLIC visuals )

