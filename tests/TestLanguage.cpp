
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
