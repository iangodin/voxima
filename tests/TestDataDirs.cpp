#include <catch2/catch_test_macros.hpp>

#include <utils/DataDirs.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
const char *const VARIABLE = "VOXIMA_TEST_DATA_DIR";

void set_variable( const std::filesystem::path &value )
{
#ifdef _WIN32
	_putenv_s( VARIABLE, value.string().c_str() );
#else
	setenv( VARIABLE, value.string().c_str(), 1 );
#endif
}

// Three folders to search, and the variable unset; all of it is removed at the end
struct Folders
{
	std::filesystem::path root = std::filesystem::temp_directory_path() / "voxima-test-data-dirs";
	std::filesystem::path local = root / "local";
	std::filesystem::path environment = root / "environment";
	std::filesystem::path installed = root / "installed";
	DataPlaces places{ local, VARIABLE, installed };

	Folders()
	{
		std::filesystem::remove_all( root );
		std::filesystem::create_directories( root );
		set_variable( "" );
	}

	~Folders()
	{
		set_variable( "" );
		std::error_code error;
		std::filesystem::remove_all( root, error );
	}

	static void add_file( const std::filesystem::path &dir, const std::string &name )
	{
		std::filesystem::create_directories( dir );
		std::ofstream( dir / name ) << "data";
	}
};
}

TEST_CASE( "A data folder is found in order: here, the environment, then installed", "[datadirs]" )
{
	Folders folders;
	CHECK_FALSE( find_data_dir( folders.places ) );

	std::filesystem::create_directories( folders.installed );
	CHECK( find_data_dir( folders.places ) == folders.installed );

	std::filesystem::create_directories( folders.environment );
	set_variable( folders.environment );
	CHECK( find_data_dir( folders.places ) == folders.environment );

	std::filesystem::create_directories( folders.local );
	CHECK( find_data_dir( folders.places ) == folders.local );
}

TEST_CASE( "A data folder named by the environment is used even if it is not there", "[datadirs]" )
{
	Folders folders;
	std::filesystem::create_directories( folders.installed );
	set_variable( folders.environment );
	CHECK( find_data_dir( folders.places ) == folders.environment );
}

TEST_CASE( "A data file is found in the first folder that has it", "[datadirs]" )
{
	Folders folders;
	const std::string file = "model.gguf";
	CHECK_FALSE( find_data_file( folders.places, file ) );

	// A folder without the file does not hide one with it
	std::filesystem::create_directories( folders.local );
	set_variable( folders.environment );
	std::filesystem::create_directories( folders.environment );
	Folders::add_file( folders.installed, file );
	CHECK( find_data_file( folders.places, file ) == folders.installed / file );

	Folders::add_file( folders.environment, file );
	CHECK( find_data_file( folders.places, file ) == folders.environment / file );

	Folders::add_file( folders.local, file );
	CHECK( find_data_file( folders.places, file ) == folders.local / file );
}

TEST_CASE( "The standard places are under share/voxima next to the program", "[datadirs]" )
{
	const DataPlaces places = DataPlaces::standard( "models", "VOXIMA_MODELS", "models" );
	CHECK( places.local == "models" );
	CHECK( places.environment_variable == "VOXIMA_MODELS" );
	REQUIRE_FALSE( places.installed.empty() );
	CHECK( places.installed.filename() == "models" );
	CHECK( places.installed.parent_path().filename() == "voxima" );
	CHECK( places.installed.parent_path().parent_path().filename() == "share" );
}
