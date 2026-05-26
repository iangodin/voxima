
cmake_minimum_required( VERSION 3.31 )

project( Sonimation )

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

if ( WIN32 )
	add_compile_definitions( WIN32_LEAN_AND_MEAN NOGDI _WIN32_WINNT=0x0601 )
	add_compile_options( /utf-8 /EHsc /W4 )
else()
	add_compile_options( -Werror -Wall -Wextra -Wno-missing-field-initializers )
endif()

find_package( lunasvg 3.5.0 REQUIRED SYSTEM )
find_package( plutovg 1.3.2 REQUIRED SYSTEM )
find_package( harfbuzz 12.3.0 REQUIRED SYSTEM )
find_package( Freetype 2.14.1 REQUIRED SYSTEM )
find_package( Catch2 3.13.0 REQUIRED SYSTEM )
find_package( CLI11 2.6.2 REQUIRED SYSTEM )
find_package( ctre 3.10.0 REQUIRED SYSTEM )
find_package( asio 1.36.0 REQUIRED SYSTEM )
find_package( CapnProto 1.1.0 REQUIRED SYSTEM )
find_package( SndFile 1.2.2 REQUIRED SYSTEM )
find_package( soxr 0.1.3 REQUIRED SYSTEM )
find_package( fribidi 1.0.13 REQUIRED SYSTEM )
find_package( clipper2 2 REQUIRED SYSTEM )

enable_testing()

add_library( visuals STATIC
	src/core/EaseCubic.cpp
	src/core/EaseElastic.cpp
	src/core/EaseBack.cpp
	src/core/EaseLinear.cpp
	src/core/Script.cpp
	src/core/Producer.cpp
	src/core/Theme.cpp
	src/core/Narration.cpp
	src/core/Narrator.cpp
	src/exporters/SVGWriter.cpp
	src/exporters/FfmpegMux.cpp
	src/exporters/FfmpegRender.cpp
	src/fonts/FontStack.cpp
	src/fonts/MathFont.cpp
	src/themes/NewComputerModern.cpp
	src/themes/AcademicClassic.cpp
	src/themes/EngineeringBlueprint.cpp
	src/themes/ModernTechnical.cpp
	src/themes/PlayfulDiscovery.cpp
	src/themes/ThemeUtils.cpp
	src/tts/protocol.capnp.c++
	src/tts/TTSClient.cpp
	src/audio/DubbingStage.cpp
	src/audio/Resample.cpp
	src/audio/AudioMixer.cpp
	src/audio/Microphone.cpp
	src/utils/FreetypeError.cpp
	src/utils/Huml.cpp
	src/visuals/CreateMathLargeOperation.cpp
	src/visuals/CreateText.cpp
	src/visuals/CreateStyledText.cpp
	src/visuals/StyledText.cpp
	src/visuals/Polyline.cpp
)

target_compile_features( visuals PUBLIC cxx_std_20 )
if ( NOT MSVC )
	target_compile_options( visuals PUBLIC -fcoroutines )
endif()
target_include_directories( visuals PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/extern ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
target_link_libraries( visuals PUBLIC
	lunasvg::lunasvg
	plutovg::plutovg
	Freetype::Freetype
	harfbuzz::harfbuzz
	CLI11::CLI11
	asio::asio
	CapnProto::capnp
	CapnProto::kj
	SndFile::sndfile
	soxr::soxr
	fribidi::fribidi
	clipper2::clipper2
)

add_executable( sonimation tools/main.cpp demos/eases/show_animation.cpp )
target_compile_features( sonimation PUBLIC cxx_std_20 )
if ( NOT MSVC )
	target_compile_options( sonimation PUBLIC -fcoroutines )
endif()
target_link_libraries( sonimation PUBLIC visuals )

add_executable( list_font tools/list_font.cpp )
target_compile_features( list_font PUBLIC cxx_std_20 )
target_link_libraries( list_font PUBLIC Freetype::Freetype )

add_executable( font_math_names tools/font_math_names.cpp )
target_compile_features( font_math_names PUBLIC cxx_std_20 )
target_link_libraries( font_math_names PUBLIC Freetype::Freetype )

add_executable( test_huml tools/test_huml.cpp )
target_compile_features( test_huml PUBLIC cxx_std_20 )
target_link_libraries( test_huml PUBLIC visuals )

add_executable( test_tts tools/test_tts.cpp )
target_compile_features( test_tts PUBLIC cxx_std_20 )
target_link_libraries( test_tts PUBLIC visuals )

add_subdirectory( tests )
