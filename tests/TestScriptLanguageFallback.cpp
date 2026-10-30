
#include <catch2/catch_test_macros.hpp>

#include <core/Script.h>

#include <filesystem>
#include <fstream>

namespace
{

// A throwaway script directory with just a base "en.huml" - no "en-CA.huml"
// exists at all, which is exactly the case BCP-47 fallback needs to handle
// (as opposed to "en-GB.huml", which exists in the real demo scripts and
// already has its own explicit meta:: inherit: "en").
struct ScriptDirFixture
{
	std::filesystem::path dir;

	ScriptDirFixture()
	{
		dir = std::filesystem::temp_directory_path() /
			( "canim-script-test-" + std::to_string( reinterpret_cast<uintptr_t>( this ) ) );
		std::filesystem::create_directories( dir );

		std::ofstream en( dir / "en.huml" );
		en << "meta::\n  lang: \"en\"\n\nvoices::\n  narrator: \"en-alba-mackenna-casual.wav\"\n";
	}

	~ScriptDirFixture()
	{
		std::filesystem::remove_all( dir );
	}
};

}

TEST_CASE( "Script::load falls back to the primary subtag when no file exists for the exact tag", "[script][language]" )
{
	ScriptDirFixture fixture;
	Script script;
	script.set_directory( fixture.dir );

	SECTION( "exact match still loads directly when the file exists" )
	{
		CHECK( script.load( "en" ) );
	}

	SECTION( "a regional variant with no dedicated file falls back to the base language" )
	{
		REQUIRE( script.load( "en-CA" ) );

		// Content came from en.huml's fallback, but stays keyed on the tag
		// that was actually requested, not silently rewritten to "en".
		const auto &voice = script.fetch( "en-CA", "voices", "narrator" );
		CHECK( voice.as_string() == "en-alba-mackenna-casual.wav" );
	}

	SECTION( "a tag with no fallback candidate at all fails to load" )
	{
		CHECK_FALSE( script.load( "xx-YY" ) );
	}
}

TEST_CASE( "Script::select_languages picks which loaded languages are in use", "[script][language]" )
{
	ScriptDirFixture fixture;
	{
		std::ofstream fr( fixture.dir / "fr.huml" );
		fr << "meta::\n  lang: \"fr\"\n\nvoices::\n  narrator: \"fr-cml-tts.wav\"\n";
	}

	Script script;
	script.set_directory( fixture.dir );
	REQUIRE( script.load( "en" ) );
	REQUIRE( script.load( "fr" ) );
	REQUIRE( script.languages() == std::vector<std::string>{ "en", "fr" } );
	CHECK( script.master_language() == "en" );

	SECTION( "one language on its own, and it is the master" )
	{
		script.select_languages( { "fr" } );
		CHECK( script.languages() == std::vector<std::string>{ "fr" } );
		CHECK( script.master_language() == "fr" );
		// the text is still there for the ones not in use
		CHECK( script.fetch( "en", "voices", "narrator" ).as_string() == "en-alba-mackenna-casual.wav" );
	}

	SECTION( "and back to all of them, in any order" )
	{
		script.select_languages( { "fr" } );
		script.select_languages( { "fr", "en" } );
		CHECK( script.languages() == std::vector<std::string>{ "fr", "en" } );
		CHECK( script.master_language() == "fr" );
	}

	SECTION( "loading one that is already loaded puts it back in use" )
	{
		script.select_languages( { "fr" } );
		REQUIRE( script.load( "en" ) );
		CHECK( script.languages() == std::vector<std::string>{ "fr", "en" } );
	}

	SECTION( "a language that was never loaded, or none at all, is an error" )
	{
		CHECK_THROWS_AS( script.select_languages( { "de" } ), std::invalid_argument );
		CHECK_THROWS_AS( script.select_languages( { "en", "de" } ), std::invalid_argument );
		CHECK_THROWS_AS( script.select_languages( {} ), std::invalid_argument );
		// nothing changed
		CHECK( script.languages() == std::vector<std::string>{ "en", "fr" } );
	}
}
