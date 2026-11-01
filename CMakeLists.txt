
cmake_minimum_required( VERSION 3.31 )

project( Voxima VERSION 0.1.0 )

set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

enable_testing()

# Vendored via `extern/audio.cpp` (squashed git subtree, see doc/internal/tts.md). Added
# before voxima's own -Werror/-Wall/-Wextra below - those flags are directory
# scoped and would otherwise leak into audio.cpp's own (not warning-clean
# under our flag set) subdirectory build.
#
# The TTS is CPU-only by default. VOXIMA_TTS_CUDA=ON builds the GPU backend if
# the CUDA toolkit is found (otherwise it still falls back to CPU-only).
option( VOXIMA_TTS_CUDA "Build audio.cpp's CUDA backend when a toolkit is available" OFF )
if ( VOXIMA_TTS_CUDA )
	find_package( CUDAToolkit QUIET )
endif()
if ( VOXIMA_TTS_CUDA AND CUDAToolkit_FOUND AND NOT CMAKE_CUDA_COMPILER )
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
if ( VOXIMA_TTS_CUDA AND CUDAToolkit_FOUND AND CMAKE_CUDA_COMPILER )
	message( STATUS "CUDA toolkit ${CUDAToolkit_VERSION} found (nvcc: ${CMAKE_CUDA_COMPILER}) - building TTS with GPU backend" )
	set( ENGINE_ENABLE_CUDA ON CACHE BOOL "" FORCE )
else()
	message( STATUS "CUDA toolkit not found (or disabled) - TTS will be CPU-only" )
	set( ENGINE_ENABLE_CUDA OFF CACHE BOOL "" FORCE )
endif()
set( ENGINE_ENABLE_HIP OFF CACHE BOOL "" FORCE )
set( ENGINE_ENABLE_VULKAN OFF CACHE BOOL "" FORCE )
set( ENGINE_ENABLE_METAL OFF CACHE BOOL "" FORCE )
if ( APPLE )
	# Apple's clang has no OpenMP (it would need Homebrew's libomp), and audio.cpp makes it a requirement unless this is
	# off. ggml does its own threading, so the speech still uses all the cores.
	set( ENGINE_ENABLE_OPENMP OFF CACHE BOOL "" FORCE )
	set( GGML_OPENMP OFF CACHE BOOL "" FORCE )
endif()
set( AUDIOCPP_BUILD_C_API ON CACHE BOOL "" FORCE )
set( AUDIOCPP_MODEL_SET "custom" CACHE STRING "" FORCE )
# chatterbox: narration synthesis. qwen3_forced_aligner: optional word- and
# subtitle-timing pass (see doc/internal/tts.md) - a separate, standalone family from
# qwen3_tts/qwen3_asr, so this doesn't pull those in.
set( AUDIOCPP_MODELS "chatterbox,qwen3_forced_aligner" CACHE STRING "" FORCE )
add_subdirectory( extern/audio.cpp )

if ( WIN32 )
	add_compile_definitions( WIN32_LEAN_AND_MEAN NOGDI _WIN32_WINNT=0x0601 )
	add_compile_options( /utf-8 /EHsc /W4 )
else()
	add_compile_options( -Werror -Wall -Wextra )
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
# SDL opens these sound libraries itself as it runs (dlopen), and only if the machine has them, so nothing links to
# them. Conan's packages still say to: that would make a program need Conan's copies. Also, libpulsecommon exports
# the libsndfile it is built with, which would be used instead of ours.
foreach( sdl_opens pulseaudio::pulseaudio sndio::sndio )
	if ( TARGET ${sdl_opens} )
		set_property( TARGET ${sdl_opens} PROPERTY INTERFACE_LINK_LIBRARIES "" )
	endif()
endforeach()
find_package( tree-sitter 0.26 REQUIRED SYSTEM )
find_package( tree-sitter-c REQUIRED SYSTEM )
find_package( tree-sitter-cpp REQUIRED SYSTEM )

# The built-in themes (themes/*.huml), compiled in (see doc/themes.md)
file( GLOB THEME_FILES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/themes/*.huml )
set( BUILTIN_THEME_SOURCES "" )
foreach( theme_file ${THEME_FILES} )
	file( READ ${theme_file} theme_text )
	string( APPEND BUILTIN_THEME_SOURCES "R\"huml(${theme_text})huml\",\n" )
	set_property( DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${theme_file} )
endforeach()
configure_file( src/core/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/core/Version.h @ONLY )
configure_file( src/core/BuiltinThemes.h.in ${CMAKE_CURRENT_BINARY_DIR}/core/BuiltinThemes.h @ONLY )

# The grammars' highlight queries, compiled in (see extern/tree-sitter-queries/README.md)
file( READ extern/tree-sitter-queries/c/highlights.scm TS_C_HIGHLIGHTS )
file( READ extern/tree-sitter-queries/cpp/highlights.scm TS_CPP_HIGHLIGHTS )
configure_file( src/code/Queries.h.in ${CMAKE_CURRENT_BINARY_DIR}/code/Queries.h @ONLY )
set_property( DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
	extern/tree-sitter-queries/c/highlights.scm
	extern/tree-sitter-queries/cpp/highlights.scm )

# The core, shared by the program and the plugins of scenes it loads (see src/core/Plugin.h): one library,
# so there is one of each registry
add_library( visuals SHARED
	src/core/PluginLoader.cpp
	src/utils/ExecutablePath.cpp
	src/core/EaseCubic.cpp
	src/core/EaseElastic.cpp
	src/core/EaseBack.cpp
	src/core/EaseLinear.cpp
	src/core/Script.cpp
	src/core/Producer.cpp
	src/core/Wait.cpp
	src/core/CameraMove.cpp
	src/code/Highlight.cpp
	src/code/CreateCode.cpp
	src/code/Typing.cpp
	src/code/LineHighlight.cpp
	src/plot/Axes.cpp
	src/plot/DrawAxes.cpp
	src/plot/NumberLine.cpp
	src/plot/Scatter.cpp
	src/plot/BarChart.cpp
	src/plot/Legend.cpp
	src/plot/Graph.cpp
	src/core/ProducerPreview.cpp
	src/core/Theme.cpp
	src/core/Palette.cpp
	src/core/ThemeFile.cpp
	src/core/Narration.cpp
	src/core/Narrator.cpp
	src/core/SceneRegistry.cpp
	src/core/WordEffects.cpp
	src/visuals/Outline.cpp
	src/visuals/Placement.cpp
	src/visuals/Panel.cpp
	src/visuals/Connector.cpp
	src/visuals/Callout.cpp
	src/visuals/Brace.cpp
	src/visuals/Geometry.cpp
	src/visuals/Diagram.cpp
	src/core/Animate.cpp
	src/visuals/Trim.cpp
	src/visuals/WrapText.cpp
	src/visuals/CreateList.cpp
	src/visuals/CreateTable.cpp
	src/visuals/MissingGlyph.cpp
	src/visuals/MorphMatch.cpp
	src/visuals/MorphLeaves.cpp
	src/visuals/Morph.cpp
	src/exporters/SVGWriter.cpp
	src/exporters/FfmpegMux.cpp
	src/exporters/FfmpegRender.cpp
	src/exporters/PngRender.cpp
	src/exporters/SubtitleWriter.cpp
	src/fonts/FontStack.cpp
	src/fonts/FontFamily.cpp
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
	src/audio/Sound.cpp
	src/audio/SoundFile.cpp
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
	src/math/Operators.cpp
	src/visuals/LargeOperator.cpp
)

add_library( Voxima::core ALIAS visuals )
set_target_properties( visuals PROPERTIES
	OUTPUT_NAME voxima_core
	EXPORT_NAME core
	WINDOWS_EXPORT_ALL_SYMBOLS ON
	POSITION_INDEPENDENT_CODE ON )
target_compile_features( visuals PUBLIC cxx_std_20 )
target_include_directories( visuals PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/extern>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
	$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
	$<INSTALL_INTERFACE:include/voxima> )
# All private: no header of the core's includes a library's, so a plugin needs only the install (see
# cmake/VoximaConfig.cmake.in). The headers only name their types, with forward declarations.
target_link_libraries( visuals PRIVATE
	lunasvg::lunasvg
	Freetype::Freetype
	SndFile::sndfile
	plutovg::plutovg
	harfbuzz::harfbuzz
	CLI11::CLI11
	audiocpp
	soxr::soxr
	fribidi::fribidi
	clipper2::clipper2
	SDL3::SDL3
	tree-sitter::tree-sitter
	tree-sitter-c::tree-sitter-c
	tree-sitter-cpp::tree-sitter-cpp
	${CMAKE_DL_LIBS}
)
if ( NOT MSVC )
	# audiocpp.h has a "/*" inside a doc comment that -Wcomment flags; not our header to fix.
	target_compile_options( visuals PRIVATE -Wno-comment )
endif()

add_executable( voxima tools/main.cpp )
target_compile_features( voxima PUBLIC cxx_std_20 )
target_link_libraries( voxima PUBLIC visuals )

include( cmake/VoximaPlugin.cmake )

# The examples, as plugins in build/Release/plugins (see examples/CMakeLists.txt)
add_subdirectory( examples )

add_executable( list_font tools/list_font.cpp )
target_compile_features( list_font PUBLIC cxx_std_20 )
target_link_libraries( list_font PUBLIC Freetype::Freetype )

add_executable( font_math_names tools/font_math_names.cpp )
target_compile_features( font_math_names PUBLIC cxx_std_20 )
target_link_libraries( font_math_names PUBLIC Freetype::Freetype )

add_executable( test_huml tools/test_huml.cpp )
target_compile_features( test_huml PUBLIC cxx_std_20 )
target_link_libraries( test_huml PUBLIC visuals )

# Windows has no rpath: audio.cpp puts audiocpp.dll in bin/, so copy it (and any other shared library) next to the program
function( copy_runtime_dlls program )
	if ( WIN32 )
		add_custom_command( TARGET ${program} POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:${program}> $<TARGET_FILE_DIR:${program}>
			COMMAND_EXPAND_LISTS )
	endif()
endfunction()

foreach( program voxima test_huml )
	copy_runtime_dlls( ${program} )
endforeach()

# Installing (make install PREFIX=<dir>): the program, the core and its headers, and what a plugin project needs to
# find them (find_package( Voxima ), then voxima_add_plugin, see examples/). Only this component:
# audio.cpp has install rules of its own (ggml's headers and libraries), which are not wanted.
include( GNUInstallDirs )
include( CMakePackageConfigHelpers )

if ( NOT WIN32 )
	# "The directory of the file itself": $ORIGIN is ELF's name for it, and dyld does not know it
	if ( APPLE )
		set( VOXIMA_ORIGIN "@loader_path" )
	else()
		set( VOXIMA_ORIGIN "$ORIGIN" )
	endif()
	set_target_properties( voxima PROPERTIES INSTALL_RPATH "${VOXIMA_ORIGIN}/../${CMAKE_INSTALL_LIBDIR}" )
	set_target_properties( visuals PROPERTIES INSTALL_RPATH "${VOXIMA_ORIGIN}" )
endif()

install( TARGETS voxima visuals EXPORT VoximaTargets COMPONENT Voxima
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} )
# The core's TTS, next to it. Not part of the interface, but a shared library it links to has to be exported with it.
install( TARGETS audiocpp EXPORT VoximaTargets COMPONENT Voxima
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
install( DIRECTORY src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/voxima COMPONENT Voxima FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" )
install( FILES ${CMAKE_CURRENT_BINARY_DIR}/core/Version.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/voxima/core COMPONENT Voxima )

set( VOXIMA_CMAKE_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/Voxima )
install( EXPORT VoximaTargets NAMESPACE Voxima:: DESTINATION ${VOXIMA_CMAKE_DIR} COMPONENT Voxima )
configure_package_config_file( cmake/VoximaConfig.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/VoximaConfig.cmake
	INSTALL_DESTINATION ${VOXIMA_CMAKE_DIR} )
# Plugins are built just as the program was, so only this version will do (see src/core/Plugin.h)
write_basic_package_version_file( ${CMAKE_CURRENT_BINARY_DIR}/VoximaConfigVersion.cmake COMPATIBILITY ExactVersion )
install( FILES
	${CMAKE_CURRENT_BINARY_DIR}/VoximaConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/VoximaConfigVersion.cmake
	cmake/VoximaPlugin.cmake
	DESTINATION ${VOXIMA_CMAKE_DIR} COMPONENT Voxima )

# The fonts are not installed (they are big): download_fonts.py <prefix>/share/voxima/fonts gets them, and
# voxima looks there
install( PROGRAMS download_fonts.py download_models.py DESTINATION ${CMAKE_INSTALL_DATADIR}/voxima COMPONENT Voxima )

# How to use it, and a plugin project to start from
install( DIRECTORY doc/html/ DESTINATION ${CMAKE_INSTALL_DOCDIR}/manual COMPONENT Voxima )
install( DIRECTORY examples/ DESTINATION ${CMAKE_INSTALL_DATADIR}/voxima/examples COMPONENT Voxima )

# The licenses: Voxima's, and those of what it is built with, in share/doc/Voxima/licenses (see
# THIRD_PARTY_NOTICES.md)
install( FILES LICENSE THIRD_PARTY_NOTICES.md CHANGELOG.md DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT Voxima )
set( VOXIMA_LICENSES ${CMAKE_INSTALL_DOCDIR}/licenses )
set( AUDIOCPP_EXTERNAL extern/audio.cpp/external )
foreach( license
		"extern/audio.cpp/LICENSE|audio.cpp"
		"${AUDIOCPP_EXTERNAL}/ggml/LICENSE|audio.cpp/ggml"
		"${AUDIOCPP_EXTERNAL}/sentencepiece/LICENSE|audio.cpp/sentencepiece"
		"${AUDIOCPP_EXTERNAL}/sentencepiece/third_party/absl/LICENSE|audio.cpp/sentencepiece/absl"
		"${AUDIOCPP_EXTERNAL}/sentencepiece/third_party/protobuf-lite/LICENSE|audio.cpp/sentencepiece/protobuf-lite"
		"${AUDIOCPP_EXTERNAL}/sentencepiece/third_party/darts_clone/LICENSE|audio.cpp/sentencepiece/darts_clone"
		"${AUDIOCPP_EXTERNAL}/sentencepiece/third_party/esaxx/LICENSE|audio.cpp/sentencepiece/esaxx"
		"${AUDIOCPP_EXTERNAL}/cJSON/LICENSE|audio.cpp/cJSON"
		"${AUDIOCPP_EXTERNAL}/cpp-httplib/LICENSE|audio.cpp/cpp-httplib"
		"${AUDIOCPP_EXTERNAL}/libyaml/License|audio.cpp/libyaml"
		"extern/tree-sitter-queries/c/LICENSE|tree-sitter-queries/c"
		"extern/tree-sitter-queries/cpp/LICENSE|tree-sitter-queries/cpp" )
	string( REPLACE "|" ";" license "${license}" )
	list( GET license 0 file )
	list( GET license 1 directory )
	install( FILES ${file} DESTINATION ${VOXIMA_LICENSES}/${directory} COMPONENT Voxima )
endforeach()
# The Conan packages' own, which the recipe collects next to the toolchain it makes (see conanfile.py)
if ( CMAKE_TOOLCHAIN_FILE )
	# (it can be relative to the build directory, as Conan's presets give it)
	cmake_path( ABSOLUTE_PATH CMAKE_TOOLCHAIN_FILE BASE_DIRECTORY ${CMAKE_BINARY_DIR} OUTPUT_VARIABLE VOXIMA_TOOLCHAIN )
	cmake_path( GET VOXIMA_TOOLCHAIN PARENT_PATH VOXIMA_GENERATORS_DIR )
endif()
if ( VOXIMA_GENERATORS_DIR AND IS_DIRECTORY "${VOXIMA_GENERATORS_DIR}/licenses" )
	install( DIRECTORY "${VOXIMA_GENERATORS_DIR}/licenses/" DESTINATION ${VOXIMA_LICENSES}/conan COMPONENT Voxima )
else()
	message( WARNING "The Conan packages' licenses were not found (run make config again): they will not be installed" )
endif()

# The compiler's runtime, for a machine that does not have it (on Windows: MSVC's, and the OpenMP that audio.cpp's
# TTS uses). InstallRequiredSystemLibraries does not know the newest Visual Studio (18, toolset v145) yet, so with
# MSVC the redistributable DLLs are found from the compiler itself, in the newest of its VC/Redist/MSVC/<version>.
set( VOXIMA_RUNTIME_DLLS "" )
if ( MSVC )
	string( REGEX REPLACE "/Tools/MSVC/.*$" "" VOXIMA_VC_DIR "${CMAKE_CXX_COMPILER}" )
	string( TOLOWER "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" VOXIMA_ARCH )
	file( GLOB VOXIMA_CRT_DIRS LIST_DIRECTORIES true "${VOXIMA_VC_DIR}/Redist/MSVC/*/${VOXIMA_ARCH}/Microsoft.VC*.CRT" )
	if ( VOXIMA_CRT_DIRS )
		list( SORT VOXIMA_CRT_DIRS COMPARE NATURAL )
		list( GET VOXIMA_CRT_DIRS -1 VOXIMA_CRT_DIR )
		string( REGEX REPLACE "\\.CRT$" ".OpenMP" VOXIMA_OPENMP_DIR "${VOXIMA_CRT_DIR}" )
		file( GLOB VOXIMA_RUNTIME_DLLS "${VOXIMA_CRT_DIR}/*.dll" "${VOXIMA_OPENMP_DIR}/vcomp140.dll" )
		list( FILTER VOXIMA_RUNTIME_DLLS EXCLUDE REGEX "vccorlib" ) # C++/CX's, not used
	endif()
endif()
if ( VOXIMA_RUNTIME_DLLS )
	install( FILES ${VOXIMA_RUNTIME_DLLS} DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Voxima )
else()
	set( CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT Voxima )
	set( CMAKE_INSTALL_OPENMP_LIBRARIES ON )
	include( InstallRequiredSystemLibraries )
endif()

# Packages of what is installed (make package, into build/packages): a .zip on Windows, a .tar.gz and a .deb
# elsewhere; -DCPACK_GENERATOR=... for others. The file name says the compiler, as plugins have to be built with the
# same one (see src/core/Plugin.h).
string( REGEX MATCH "^[0-9]+\\.[0-9]+" VOXIMA_COMPILER_VERSION "${CMAKE_CXX_COMPILER_VERSION}" )
string( TOLOWER "${CMAKE_CXX_COMPILER_ID}${VOXIMA_COMPILER_VERSION}" VOXIMA_COMPILER )
string( TOLOWER "${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}" VOXIMA_SYSTEM )
set( CPACK_PACKAGE_NAME voxima )
set( CPACK_PACKAGE_VENDOR "Voxima" )
set( CPACK_PACKAGE_CONTACT "Ian Godin" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Educational videos from C++ scenes: vector graphics, narration and subtitles in many languages" )
set( CPACK_PACKAGE_FILE_NAME "voxima-${PROJECT_VERSION}-${VOXIMA_SYSTEM}-${VOXIMA_COMPILER}" )
# For the installers that show it (some want it to be .txt)
configure_file( LICENSE ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt COPYONLY )
set( CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_BINARY_DIR}/LICENSE.txt )
if ( NOT CPACK_GENERATOR )
	if ( WIN32 )
		set( CPACK_GENERATOR ZIP )
	else()
		set( CPACK_GENERATOR TGZ DEB )
	endif()
endif()
# Only Voxima's own (see above), in one package
set( CPACK_COMPONENTS_ALL Voxima )
set( CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE )
set( CPACK_ARCHIVE_COMPONENT_INSTALL ON )
set( CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY ON ) # unpacked, it is one folder
set( CPACK_DEB_COMPONENT_INSTALL ON )
set( CPACK_DEBIAN_PACKAGE_DEPENDS "ffmpeg" ) # it renders through ffmpeg
find_program( DPKG_SHLIBDEPS dpkg-shlibdeps )
if ( DPKG_SHLIBDEPS )
	set( CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON ) # and the system libraries it links to
endif()
include( CPack )

add_subdirectory( tests )
