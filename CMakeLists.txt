
cmake_minimum_required( VERSION 3.31 )

project( Sonimation )

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

enable_testing()

# Vendored via `extern/audio.cpp` (squashed git subtree, see tts.md). Added
# before canim's own -Werror/-Wall/-Wextra below - those flags are directory
# scoped and would otherwise leak into audio.cpp's own (not warning-clean
# under our flag set) subdirectory build.
#
# CUDA is auto-detected: if the toolkit is present we build the GPU backend,
# otherwise this falls back to CPU-only. CANIM_TTS_CUDA=OFF forces CPU-only
# even when a toolkit is present.
option( CANIM_TTS_CUDA "Build audio.cpp's CUDA backend when a toolkit is available" ON )
if ( CANIM_TTS_CUDA )
	find_package( CUDAToolkit QUIET )
endif()
if ( CANIM_TTS_CUDA AND CUDAToolkit_FOUND AND NOT CMAKE_CUDA_COMPILER )
	# find_package(CUDAToolkit) locates the toolkit independently of PATH,
	# but enable_language(CUDA) (triggered by audio.cpp below) still needs
	# nvcc findable via PATH or CMAKE_CUDA_COMPILER - nvcc frequently isn't
	# on PATH even when the toolkit itself is installed. /usr/local/cuda is
	# the default install location on nearly every Linux distro, so fall
	# back to it explicitly rather than requiring the caller to set
	# CUDACXX/PATH themselves.
	find_program( CMAKE_CUDA_COMPILER NAMES nvcc
		PATHS ${CUDAToolkit_BIN_DIR} /usr/local/cuda/bin
		NO_DEFAULT_PATH )
endif()
if ( CANIM_TTS_CUDA AND CUDAToolkit_FOUND AND CMAKE_CUDA_COMPILER )
	message( STATUS "CUDA toolkit ${CUDAToolkit_VERSION} found (nvcc: ${CMAKE_CUDA_COMPILER}) - building TTS with GPU backend" )
	set( ENGINE_ENABLE_CUDA ON CACHE BOOL "" FORCE )
else()
	message( STATUS "CUDA toolkit not found (or disabled) - TTS will be CPU-only" )
	set( ENGINE_ENABLE_CUDA OFF CACHE BOOL "" FORCE )
endif()
set( ENGINE_ENABLE_HIP OFF CACHE BOOL "" FORCE )
set( ENGINE_ENABLE_VULKAN OFF CACHE BOOL "" FORCE )
set( ENGINE_ENABLE_METAL OFF CACHE BOOL "" FORCE )
set( AUDIOCPP_BUILD_C_API ON CACHE BOOL "" FORCE )
set( AUDIOCPP_MODEL_SET "custom" CACHE STRING "" FORCE )
# chatterbox: narration synthesis. qwen3_forced_aligner: optional subtitle-
# timing pass (see tts.md phase 4) - a separate, standalone family from
# qwen3_tts/qwen3_asr, so this doesn't pull those in.
set( AUDIOCPP_MODELS "chatterbox,qwen3_forced_aligner" CACHE STRING "" FORCE )
add_subdirectory( extern/audio.cpp )

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
find_package( SndFile 1.2.2 REQUIRED SYSTEM )
find_package( soxr 0.1.3 REQUIRED SYSTEM )
find_package( fribidi 1.0.13 REQUIRED SYSTEM )
find_package( clipper2 2 REQUIRED SYSTEM )
find_package( SDL3 REQUIRED SYSTEM )

add_library( visuals STATIC
	src/core/EaseCubic.cpp
	src/core/EaseElastic.cpp
	src/core/EaseBack.cpp
	src/core/EaseLinear.cpp
	src/core/Script.cpp
	src/core/Producer.cpp
	src/core/ProducerPreview.cpp
	src/core/Theme.cpp
	src/core/Narration.cpp
	src/core/Narrator.cpp
	src/core/SceneRegistry.cpp
	src/core/WordEffects.cpp
	src/visuals/Outline.cpp
	src/visuals/MissingGlyph.cpp
	src/visuals/MorphMatch.cpp
	src/visuals/MorphLeaves.cpp
	src/visuals/Morph.cpp
	src/exporters/SVGWriter.cpp
	src/exporters/FfmpegMux.cpp
	src/exporters/FfmpegRender.cpp
	src/exporters/SubtitleWriter.cpp
	src/fonts/FontStack.cpp
	src/fonts/MathFont.cpp
	src/fonts/MathAlphabet.cpp
	src/themes/NewComputerModern.cpp
	src/themes/AcademicClassic.cpp
	src/themes/EngineeringBlueprint.cpp
	src/themes/ModernTechnical.cpp
	src/themes/PlayfulDiscovery.cpp
	src/themes/ThemeUtils.cpp
	src/tts/TTSClient.cpp
	src/audio/DubbingStage.cpp
	src/audio/Resample.cpp
	src/audio/Retime.cpp
	src/audio/Stretch.cpp
	src/audio/Compress.cpp
	src/audio/AudioMixer.cpp
	src/audio/Microphone.cpp
	src/audio/Silence.cpp
	src/audio/Volume.cpp
	src/utils/FreetypeError.cpp
	src/utils/Huml.cpp
	src/utils/Language.cpp
	src/visuals/CreateMathLargeOperation.cpp
	src/visuals/CreateText.cpp
	src/visuals/CreateStyledText.cpp
	src/visuals/StyledText.cpp
	src/visuals/Polyline.cpp
	src/visuals/Radical.cpp
	src/visuals/Fraction.cpp
	src/visuals/Scripts.cpp
	src/visuals/Row.cpp
	src/visuals/Limits.cpp
	src/visuals/Delimited.cpp
	src/visuals/Accent.cpp
	src/visuals/Bars.cpp
	src/visuals/Matrix.cpp
	src/math/Context.cpp
	src/math/Expr.cpp
	src/visuals/LargeOperator.cpp
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
	audiocpp
	SndFile::sndfile
	soxr::soxr
	fribidi::fribidi
	clipper2::clipper2
	SDL3::SDL3
)
if ( NOT MSVC )
	# audiocpp.h has a "/*" inside a doc comment that -Wcomment flags; not our header to fix.
	target_compile_options( visuals PRIVATE -Wno-comment )
endif()

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

add_executable( tts_poc tools/tts_poc.cpp )
target_compile_features( tts_poc PUBLIC cxx_std_20 )
target_link_libraries( tts_poc PUBLIC audiocpp SndFile::sndfile )
if ( NOT MSVC )
	# audiocpp.h has a "/*" inside a doc comment that -Wcomment flags; not our header to fix.
	target_compile_options( tts_poc PRIVATE -Wno-comment )
endif()

add_executable( align_poc tools/align_poc.cpp )
target_compile_features( align_poc PUBLIC cxx_std_20 )
target_link_libraries( align_poc PUBLIC visuals )

add_subdirectory( tests )
