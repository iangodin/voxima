
#include <catch2/catch_test_macros.hpp>

#include <core/Document.h>

#include <variant>

TEST_CASE( "HUML parser line counting", "[huml]" )
{
	huml::Document doc;

	SECTION( "Valid simple document" )
	{
		auto result = doc.parse( "key: value\n" );
		CHECK( std::holds_alternative<huml::Value>( result ) );
	}

	SECTION( "Invalid quoted string reports correct line" )
	{
		auto result = doc.parse( "key: \"unclosed\n" );
		REQUIRE( std::holds_alternative<huml::ParseError>( result ) );
		CHECK( std::get<huml::ParseError>( result ).line == 2 );
	}

	SECTION( "Error on later lines" )
	{
		auto result = doc.parse( "first: 1\nsecond: 2\nthird: \"unclosed\n" );
		REQUIRE( std::holds_alternative<huml::ParseError>( result ) );
		CHECK( std::get<huml::ParseError>( result ).line == 4 );
	}

	SECTION( "Invalid escape sequence" )
	{
		auto result = doc.parse( "key: \"\\x\"\n" );
		REQUIRE( std::holds_alternative<huml::ParseError>( result ) );
		CHECK( std::get<huml::ParseError>( result ).line == 1 );
	}
}
