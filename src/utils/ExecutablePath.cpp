#include "ExecutablePath.h"

#include <cstdint>
#include <system_error>

#if defined( _WIN32 )
#define NOMINMAX
#include <windows.h>
#include <vector>
#elif defined( __APPLE__ )
#include <mach-o/dyld.h>
#include <vector>
#endif

std::filesystem::path executable_path()
{
#if defined( _WIN32 )
	std::vector<wchar_t> buffer( MAX_PATH );
	while ( true )
	{
		const DWORD length = GetModuleFileNameW( nullptr, buffer.data(), static_cast<DWORD>( buffer.size() ) );
		if ( length == 0 )
		{
			return {};
		}
		if ( length < buffer.size() )
		{
			return std::filesystem::path( std::wstring( buffer.data(), length ) );
		}
		buffer.resize( buffer.size() * 2 );
	}
#elif defined( __APPLE__ )
	uint32_t size = 0;
	_NSGetExecutablePath( nullptr, &size );
	std::vector<char> buffer( size );
	if ( _NSGetExecutablePath( buffer.data(), &size ) != 0 )
	{
		return {};
	}
	std::error_code error;
	return std::filesystem::canonical( buffer.data(), error );
#else
	std::error_code error;
	return std::filesystem::read_symlink( "/proc/self/exe", error );
#endif
}
