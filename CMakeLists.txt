
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
find_package( Catch2 REQUIRED )

enable_testing()

add_subdirectory( extern/mfl )

add_library( visuals STATIC
	src/core/Animation.h
	src/core/Director.h
	src/core/Producer.h
	src/core/Production.h
	src/core/Scene.h
	src/core/Screenplay.h
	src/core/Stage.h
	src/exporters/SVGWriter.cpp
	src/exporters/SVGWriter.h
	src/themes/Fira.cpp
	src/themes/Fira.h
	src/fonts/FontFace.h
	src/fonts/FontStack.cpp
	src/fonts/FontStack.h
	src/fonts/MathFonts.h
	src/fonts/MFLFontFace.cpp
	src/fonts/MFLFontFace.h
	src/themes/NewComputerModern.cpp
	src/themes/NewComputerModern.h
	src/fonts/Theme.cpp
	src/fonts/Theme.h
	src/fonts/TextStyle.h
	src/utils/freetype_error.cpp
	src/utils/freetype_error.hpp
	src/utils/Timer.h
	src/utils/Utf8.h
	src/visuals/CreateMath.cpp
	src/visuals/CreateMath.h
	src/visuals/CreateText.cpp
	src/visuals/CreateText.h
	src/visuals/ArrowTip.h
	src/visuals/Bounds.h
	src/visuals/Circle.h
	src/visuals/Ellipse.h
	src/visuals/Glyph.h
	src/visuals/Group.h
	src/visuals/Image.h
	src/visuals/Line.h
	src/visuals/Path.h
	src/visuals/Point.h
	src/visuals/Rectangle.h
	src/visuals/RoundRectangle.h
	src/visuals/Visual.h
	src/visuals/VisualVisitor.h
)

target_compile_features( visuals PUBLIC cxx_std_20 )
target_compile_options( visuals PUBLIC -fcoroutines )
target_include_directories( visuals PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/extern ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( visuals PUBLIC stb::stb lunasvg::lunasvg plutovg::plutovg Freetype::Freetype harfbuzz::harfbuzz mfl )

add_executable( canim tools/main.cpp )
target_compile_features( canim PUBLIC cxx_std_20 )
target_compile_options( canim PUBLIC -fcoroutines )
target_link_libraries( canim PUBLIC visuals )

add_executable( math_test tools/math_test.cpp )
target_compile_features( math_test PUBLIC cxx_std_20 )
target_compile_options( math_test PUBLIC -fcoroutines )
target_link_libraries( math_test PUBLIC visuals )

add_executable( test_font tools/test_font.cpp )
target_compile_features( test_font PUBLIC cxx_std_20 )
target_compile_options( test_font PUBLIC -fcoroutines )
target_link_libraries( test_font PUBLIC visuals )

add_executable( list_font tools/list_font.cpp )
target_compile_features( list_font PUBLIC cxx_std_20 )
target_link_libraries( list_font PUBLIC Freetype::Freetype )

add_subdirectory( tests )
