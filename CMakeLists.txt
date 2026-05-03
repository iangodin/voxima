
cmake_minimum_required( VERSION 3.31 )

project( Sonimation )

set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

if ( WIN32 )
	add_compile_options( /utf-8 /EHsc /W4 )
else()
	add_compile_options( -Werror -Wall -Wextra -Wno-missing-field-initializers )
endif()

find_package( lunasvg 3.5.0 REQUIRED )
find_package( plutovg 1.3.2 REQUIRED )
find_package( harfbuzz 12.3.0 REQUIRED )
find_package( Freetype 2.14.1 REQUIRED )
find_package( Catch2 REQUIRED )
find_package( CLI11 REQUIRED )
find_package( ctre REQUIRED )

enable_testing()

add_subdirectory( extern/mfl )

add_library( visuals STATIC
	src/core/EaseCubic.cpp
	src/core/EaseElastic.cpp
	src/core/EaseBack.cpp
	src/core/EaseLinear.cpp
	src/core/Script.cpp
	src/core/Producer.cpp
	src/core/Theme.cpp
	src/exporters/SVGWriter.cpp
	src/exporters/FfmpegRender.cpp
	src/fonts/FontStack.cpp
	src/fonts/MFLFontFace.cpp
	src/themes/NewComputerModern.cpp
	src/themes/Fira.cpp
	src/themes/AcademicClassic.cpp
	src/themes/EngineeringBlueprint.cpp
	src/themes/ModernTechnical.cpp
	src/themes/PlayfulDiscovery.cpp
	src/utils/FreetypeError.cpp
	src/utils/Huml.cpp
	src/visuals/CreateMath.cpp
	src/visuals/CreateText.cpp
)

target_compile_features( visuals PUBLIC cxx_std_20 )
target_compile_options( visuals PUBLIC -fcoroutines )
target_include_directories( visuals PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/extern ${CMAKE_CURRENT_SOURCE_DIR} )
target_link_libraries( visuals PUBLIC lunasvg::lunasvg plutovg::plutovg Freetype::Freetype harfbuzz::harfbuzz CLI11::CLI11 mfl )

add_executable( sonimation tools/main.cpp demos/eases/show_animation.cpp )
target_compile_features( sonimation PUBLIC cxx_std_20 )
target_compile_options( sonimation PUBLIC -fcoroutines )
target_link_libraries( sonimation PUBLIC visuals )

add_executable( list_font tools/list_font.cpp )
target_compile_features( list_font PUBLIC cxx_std_20 )
target_link_libraries( list_font PUBLIC Freetype::Freetype )

add_subdirectory( tests )
