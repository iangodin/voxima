#pragma once

#include <core/Version.h>

// A standard header, for the macros saying which standard library this is, and how it was built (not just any:
// MSVC's <cstddef> does not say how its iterators are checked)
#include <string>

// Scenes can be in a plugin (a DLL, or a .so), loaded with --plugin, instead of in the program. A plugin is
// its scenes, registered as usual (see SceneRegistry.h), and this line, once, in one of its files:
//
//     VOXIMA_PLUGIN();
//
// It says what the plugin was built with. Scenes share the program's strings, functions and coroutines, so a
// plugin has to be built just as the program was (the same compiler, standard library and Debug/Release), and
// against the same Voxima; one that was not is refused when it is loaded, rather than crashing.

#define VOXIMA_ABI_STR_( x ) #x
#define VOXIMA_ABI_STR( x ) VOXIMA_ABI_STR_( x )

#if defined( __clang__ )
#define VOXIMA_ABI_COMPILER "clang-" VOXIMA_ABI_STR( __clang_major__ )
#elif defined( _MSC_VER )
#define VOXIMA_ABI_COMPILER "msvc-" VOXIMA_ABI_STR( _MSC_VER )
#elif defined( __GNUC__ )
#define VOXIMA_ABI_COMPILER "gcc-" VOXIMA_ABI_STR( __GNUC__ )
#else
#define VOXIMA_ABI_COMPILER "unknown"
#endif

#if defined( _LIBCPP_VERSION )
#define VOXIMA_ABI_LIBRARY " libc++"
#elif defined( _GLIBCXX_USE_CXX11_ABI )
#define VOXIMA_ABI_LIBRARY " libstdc++-cxx11abi" VOXIMA_ABI_STR( _GLIBCXX_USE_CXX11_ABI )
#elif defined( _MSC_VER )
#define VOXIMA_ABI_LIBRARY " msvcstl"
#else
#define VOXIMA_ABI_LIBRARY ""
#endif

// Debug and Release builds have different standard library types (MSVC's checked iterators, libstdc++'s debug mode)
#if defined( _ITERATOR_DEBUG_LEVEL )
#define VOXIMA_ABI_DEBUG " iterator-debug" VOXIMA_ABI_STR( _ITERATOR_DEBUG_LEVEL )
#elif defined( _GLIBCXX_DEBUG )
#define VOXIMA_ABI_DEBUG " glibcxx-debug"
#else
#define VOXIMA_ABI_DEBUG ""
#endif

#if defined( _DEBUG )
#define VOXIMA_ABI_CRT " debug-crt"
#else
#define VOXIMA_ABI_CRT ""
#endif

// What a plugin must have been built with, to be loaded
#define VOXIMA_ABI \
	"voxima-" VOXIMA_VERSION " " VOXIMA_ABI_COMPILER VOXIMA_ABI_LIBRARY VOXIMA_ABI_DEBUG VOXIMA_ABI_CRT

#if defined( _WIN32 )
#define VOXIMA_PLUGIN_EXPORT __declspec( dllexport )
#else
#define VOXIMA_PLUGIN_EXPORT __attribute__( ( visibility( "default" ) ) )
#endif

// The name the program looks for in a plugin
#define VOXIMA_PLUGIN_ABI_FUNCTION "voxima_plugin_abi"

#define VOXIMA_PLUGIN()                                              \
	extern "C" VOXIMA_PLUGIN_EXPORT const char *voxima_plugin_abi() \
	{                                                                    \
		return VOXIMA_ABI;                                           \
	}                                                                    \
	static_assert( true )
