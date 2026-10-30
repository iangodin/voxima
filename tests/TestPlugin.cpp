#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <core/PluginLoader.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>

#include <filesystem>
#include <stdexcept>
#include <string>

// The test plugins (tests/plugin/), built next to the tests
#ifndef SONIMATION_TEST_PLUGIN
#error "SONIMATION_TEST_PLUGIN is not defined"
#endif

namespace
{
int run( Scene &scene )
{
	int frames = 0;
	while ( !scene.done() && frames < 1000 )
	{
		scene.resume();
		++frames;
	}
	return frames;
}

std::string load_error( Plugin &plugin )
{
	try
	{
		plugin.load();
	}
	catch ( const std::runtime_error &e )
	{
		return e.what();
	}
	return {};
}
}

TEST_CASE( "A plugin's scenes are registered while it is loaded", "[plugin]" )
{
	Plugin plugin( SONIMATION_TEST_PLUGIN );
	CHECK( SceneEntry::get_by_name( "plugin-test" ) == nullptr );

	plugin.load();
	REQUIRE( plugin.loaded() );
	const SceneEntry *entry = SceneEntry::get_by_name( "plugin-test" );
	REQUIRE( entry != nullptr );
	CHECK( entry->description() == "A scene from a plugin" );

	{
		// Everything the plugin made goes before it does
		Stage stage;
		Script script;
		Scene scene = play( stage, script, "plugin-test" );
		CHECK( run( scene ) >= 3 );
		CHECK( stage.visuals.size() == 1 );
	}

	plugin.unload();
	CHECK_FALSE( plugin.loaded() );
	CHECK( SceneEntry::get_by_name( "plugin-test" ) == nullptr );

	// And again, as the preview does when it is rebuilt
	plugin.load();
	CHECK( SceneEntry::get_by_name( "plugin-test" ) != nullptr );
	plugin.load();
	CHECK( SceneEntry::get_by_name( "plugin-test" ) != nullptr );
}

TEST_CASE( "A plugin can be named without its extension", "[plugin]" )
{
	std::filesystem::path path( SONIMATION_TEST_PLUGIN );
	Plugin plugin( path.parent_path() / path.stem() );
	CHECK( plugin.path() == path );
}

TEST_CASE( "A plugin that is not there is reported", "[plugin]" )
{
	CHECK_THROWS_WITH( Plugin( "no/such/plugin" ), Catch::Matchers::StartsWith( "no plugin no/such/plugin" ) );
}

TEST_CASE( "A plugin built for something else is refused, and its scenes go", "[plugin]" )
{
	Plugin plugin( SONIMATION_TEST_PLUGIN_WRONG_ABI );
	const std::string error = load_error( plugin );
	CHECK_THAT( error, Catch::Matchers::ContainsSubstring( "it was built for sonimation-0.0 some-other-compiler" ) );
	CHECK_THAT( error, Catch::Matchers::ContainsSubstring( sonimation_abi() ) );
	CHECK_FALSE( plugin.loaded() );
	CHECK( SceneEntry::get_by_name( "plugin-wrong-abi" ) == nullptr );
}

TEST_CASE( "changed() waits for a rebuilt plugin to stop changing", "[plugin]" )
{
	const auto directory = std::filesystem::temp_directory_path() / "sonimation-plugin-test";
	std::filesystem::create_directories( directory );
	const auto copy = directory / std::filesystem::path( SONIMATION_TEST_PLUGIN ).filename();
	std::filesystem::copy_file( SONIMATION_TEST_PLUGIN, copy, std::filesystem::copy_options::overwrite_existing );

	{
		Plugin plugin( copy );
		plugin.load();
		CHECK_FALSE( plugin.changed() );

		// "Rebuilt": a different size
		std::filesystem::resize_file( copy, std::filesystem::file_size( copy ) + 16 );
		CHECK_FALSE( plugin.changed() ); // just seen to change: it may still be being written
		CHECK( plugin.changed() );       // the same as last time: it is done

		plugin.load();
		CHECK_FALSE( plugin.changed() );
	}

	std::filesystem::remove_all( directory );
}
