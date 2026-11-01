
#include <catch2/catch_test_macros.hpp>

#include <utils/Language.h>

TEST_CASE( "Language: primary_subtag", "[language]" )
{
	CHECK( language::primary_subtag( "en" ) == "en" );
	CHECK( language::primary_subtag( "en-GB" ) == "en" );
	CHECK( language::primary_subtag( "en-US" ) == "en" );
	CHECK( language::primary_subtag( "fr-CA" ) == "fr" );
	CHECK( language::primary_subtag( "zh-Hans-CN" ) == "zh" );
	CHECK( language::primary_subtag( "EN-GB" ) == "en" );
	CHECK( language::primary_subtag( "DE" ) == "de" );
}

TEST_CASE( "Language: fallback_chain", "[language]" )
{
	CHECK( language::fallback_chain( "en" ) == std::vector<std::string>{ "en" } );
	CHECK( language::fallback_chain( "en-GB" ) == std::vector<std::string>{ "en-GB", "en" } );
	CHECK( language::fallback_chain( "fr-CA" ) == std::vector<std::string>{ "fr-CA", "fr" } );
	CHECK( language::fallback_chain( "zh-Hans-CN" ) == std::vector<std::string>{ "zh-Hans-CN", "zh" } );

	SECTION( "the exact requested tag is preserved verbatim, not normalized" )
	{
		auto chain = language::fallback_chain( "en-GB" );
		REQUIRE( !chain.empty() );
		CHECK( chain.front() == "en-GB" );
	}
}

TEST_CASE( "A container's language label is the three-letter code", "[language]" )
{
	CHECK( language::container_language( "en", false ) == "eng" );
	CHECK( language::container_language( "en-GB", true ) == "eng" );
	CHECK( language::container_language( "FR-ca", false ) == "fra" );
	CHECK( language::container_language( "fr", true ) == "fre" );
	CHECK( language::container_language( "de", false ) == "deu" );
	CHECK( language::container_language( "de", true ) == "ger" );
	CHECK( language::container_language( "zh-Hans-CN", false ) == "zho" );
	// One that is not known stays as it is
	CHECK( language::container_language( "xx-YY", false ) == "xx" );
}
