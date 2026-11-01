#include "DataDirs.h"

#include "ExecutablePath.h"

#include <cstdlib>
#include <system_error>
#include <utility>

namespace
{
std::optional<std::filesystem::path> environment_dir( const DataPlaces &places )
{
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 4996 ) // getenv: only read, here
#endif
	const char *value = std::getenv( places.environment_variable.c_str() );
#ifdef _MSC_VER
#pragma warning( pop )
#endif
	if ( value && *value )
	{
		return std::filesystem::path( value );
	}
	return std::nullopt;
}
}

DataPlaces DataPlaces::standard( std::filesystem::path local, std::string environment_variable, const std::string &name )
{
	DataPlaces places{ std::move( local ), std::move( environment_variable ), {} };
	if ( const auto program = executable_path(); !program.empty() )
	{
		places.installed = program.parent_path().parent_path() / "share" / "voxima" / name;
	}
	return places;
}

std::optional<std::filesystem::path> find_data_dir( const DataPlaces &places )
{
	std::error_code error;
	if ( std::filesystem::is_directory( places.local, error ) )
	{
		return places.local;
	}
	if ( auto dir = environment_dir( places ) )
	{
		return dir;
	}
	if ( !places.installed.empty() && std::filesystem::is_directory( places.installed, error ) )
	{
		return places.installed;
	}
	return std::nullopt;
}

std::optional<std::filesystem::path> find_data_file( const DataPlaces &places, const std::filesystem::path &file )
{
	std::error_code error;
	const auto has_file = [&]( const std::filesystem::path &dir )
	{
		return !dir.empty() && std::filesystem::is_regular_file( dir / file, error );
	};
	if ( has_file( places.local ) )
	{
		return places.local / file;
	}
	if ( const auto dir = environment_dir( places ); dir && has_file( *dir ) )
	{
		return *dir / file;
	}
	if ( has_file( places.installed ) )
	{
		return places.installed / file;
	}
	return std::nullopt;
}
