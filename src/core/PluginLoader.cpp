#include "PluginLoader.h"
#include "Plugin.h"

#include <atomic>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#define GETPID _getpid
#include <process.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#define GETPID getpid
#endif

namespace
{

// What CMake gives a MODULE library (on macOS too)
#if defined( _WIN32 )
constexpr const char *plugin_extension = ".dll";
#else
constexpr const char *plugin_extension = ".so";
#endif

// The file a plugin path means: as it is, or with the platform's extension added
std::filesystem::path resolve( std::filesystem::path path )
{
	std::error_code error;
	if ( std::filesystem::is_regular_file( path, error ) )
	{
		return path;
	}
	std::filesystem::path with_extension = path;
	with_extension += plugin_extension;
	if ( std::filesystem::is_regular_file( with_extension, error ) )
	{
		return with_extension;
	}
	throw std::runtime_error( "no plugin " + path.string() + " (nor " + with_extension.string() + ")" );
}

// A new name in the temporary directory for a copy of a plugin, so a loaded one is never written over
std::filesystem::path copy_path( const std::filesystem::path &path )
{
	static std::atomic<unsigned> counter{ 0 };
	const auto directory = std::filesystem::temp_directory_path() / "voxima-plugins";
	std::filesystem::create_directories( directory );

	// Copies left by a program that did not get to remove its own (it crashed, or was killed). Only old ones, which
	// are not another's that is running; and one still loaded cannot be removed on Windows anyway.
	std::error_code error;
	const auto old = std::filesystem::file_time_type::clock::now() - std::chrono::hours( 24 );
	for ( const auto &entry: std::filesystem::directory_iterator( directory, error ) )
	{
		if ( entry.last_write_time( error ) < old && !error )
		{
			std::filesystem::remove( entry.path(), error );
		}
	}

	const std::string name = path.stem().string() + "-" + std::to_string( GETPID() ) + "-" + std::to_string( counter++ );
	return directory / ( name + path.extension().string() );
}

#ifdef _WIN32
std::string last_error()
{
	return std::system_category().message( static_cast<int>( GetLastError() ) );
}
#endif

void *open_library( const std::filesystem::path &path, const std::filesystem::path &original )
{
#ifdef _WIN32
	HMODULE module = LoadLibraryW( path.wstring().c_str() );
	if ( module == nullptr )
	{
		throw std::runtime_error( "could not load plugin " + original.string() + ": " + last_error() );
	}
	return reinterpret_cast<void *>( module );
#else
	void *handle = dlopen( path.c_str(), RTLD_NOW | RTLD_LOCAL );
	if ( handle == nullptr )
	{
		throw std::runtime_error( "could not load plugin " + original.string() + ": " + dlerror() );
	}
	return handle;
#endif
}

void close_library( void *handle )
{
#ifdef _WIN32
	FreeLibrary( reinterpret_cast<HMODULE>( handle ) );
#else
	dlclose( handle );
#endif
}

void *find_symbol( void *handle, const char *name )
{
#ifdef _WIN32
	return reinterpret_cast<void *>( GetProcAddress( reinterpret_cast<HMODULE>( handle ), name ) );
#else
	return dlsym( handle, name );
#endif
}

}

std::vector<Plugin> load_plugins_in( const std::filesystem::path &directory, std::ostream &warnings )
{
	std::error_code error;
	if ( !std::filesystem::is_directory( directory, error ) )
	{
		throw std::runtime_error( "no plugin directory " + directory.string() );
	}

	std::vector<std::filesystem::path> files;
	for ( const auto &entry: std::filesystem::directory_iterator( directory ) )
	{
		if ( entry.is_regular_file( error ) && entry.path().extension() == plugin_extension )
		{
			files.push_back( entry.path() );
		}
	}
	std::sort( files.begin(), files.end() );

	std::vector<Plugin> plugins;
	for ( const auto &file: files )
	{
		try
		{
			Plugin plugin( file );
			plugin.load();
			plugins.push_back( std::move( plugin ) );
		}
		catch ( const std::exception &e )
		{
			warnings << "warning: skipping " << e.what() << "\n";
		}
	}
	return plugins;
}

const char *voxima_abi()
{
	return VOXIMA_ABI;
}

Plugin::Plugin( std::filesystem::path path ) :
	_path( resolve( std::move( path ) ) )
{
}

Plugin::~Plugin()
{
	unload();
}

Plugin::Plugin( Plugin &&other ) noexcept :
	_path( std::move( other._path ) ),
	_copy( std::move( other._copy ) ),
	_handle( std::exchange( other._handle, nullptr ) ),
	_loaded_stamp( std::move( other._loaded_stamp ) ),
	_seen_stamp( std::move( other._seen_stamp ) )
{
}

Plugin &Plugin::operator=( Plugin &&other ) noexcept
{
	if ( this != &other )
	{
		unload();
		_path = std::move( other._path );
		_copy = std::move( other._copy );
		_handle = std::exchange( other._handle, nullptr );
		_loaded_stamp = std::move( other._loaded_stamp );
		_seen_stamp = std::move( other._seen_stamp );
	}
	return *this;
}

void Plugin::load()
{
	unload();

	// The file as it is tried, loaded or not: one that fails is not changed() until it is built again
	const auto stamp_before = stamp();
	_loaded_stamp = stamp_before;
	_seen_stamp = stamp_before;

	const auto copy = copy_path( _path );
	std::filesystem::copy_file( _path, copy, std::filesystem::copy_options::overwrite_existing );

	void *handle = nullptr;
	try
	{
		handle = open_library( copy, _path );
	}
	catch ( ... )
	{
		std::error_code error;
		std::filesystem::remove( copy, error );
		throw;
	}

	// Its scenes registered as it loaded; if it was not built as the program was, they cannot be trusted, and go
	// as it is unloaded
	using AbiFunction = const char *( * )();
	auto abi = reinterpret_cast<AbiFunction>( find_symbol( handle, VOXIMA_PLUGIN_ABI_FUNCTION ) );
	std::string problem;
	if ( abi == nullptr )
	{
		problem = "it has no VOXIMA_PLUGIN() (see core/Plugin.h)";
	}
	else if ( std::string( abi() ) != voxima_abi() )
	{
		problem = std::string( "it was built for " ) + abi() + ", but this program is " + voxima_abi();
	}
	if ( !problem.empty() )
	{
		close_library( handle );
		std::error_code error;
		std::filesystem::remove( copy, error );
		throw std::runtime_error( "could not load plugin " + _path.string() + ": " + problem );
	}

	_handle = handle;
	_copy = copy;
}

void Plugin::unload()
{
	if ( _handle == nullptr )
	{
		return;
	}
	close_library( _handle );
	_handle = nullptr;

	std::error_code error;
	std::filesystem::remove( _copy, error );
	_copy.clear();
}

bool Plugin::changed()
{
	const auto now = stamp();
	if ( !now || now == _loaded_stamp )
	{
		_seen_stamp = _loaded_stamp;
		return false;
	}
	if ( now != _seen_stamp )
	{
		_seen_stamp = now;
		return false;
	}
	return true;
}

std::optional<Plugin::Stamp> Plugin::stamp() const
{
	std::error_code error;
	const auto time = std::filesystem::last_write_time( _path, error );
	if ( error )
	{
		return std::nullopt;
	}
	const auto size = std::filesystem::file_size( _path, error );
	if ( error )
	{
		return std::nullopt;
	}
	return Stamp{ time, size };
}
