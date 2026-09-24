
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
