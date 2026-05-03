
#include <catch2/catch_test_macros.hpp>

#include <utils/Huml.h>

#include <variant>

using namespace huml;

TEST_CASE( "HUML parser line counting", "[huml]" )
{

	SECTION( "Valid simple document" )
	{
		Parser doc( "key: value\n" );
		auto result = doc.parse();
		CHECK( std::holds_alternative<Value>( result ) );
	}

	SECTION( "Invalid quoted string reports correct line" )
	{
		Parser doc( "key: \"unclosed\n" );
		auto result = doc.parse();
		REQUIRE( std::holds_alternative<ParseError>( result ) );
		CHECK( std::get<ParseError>( result ).line == 2 );
	}

	SECTION( "Error on later lines" )
	{
		Parser doc( "first: 1\nsecond: 2\nthird: \"unclosed\n" );
		auto result = doc.parse();
		REQUIRE( std::holds_alternative<ParseError>( result ) );
		CHECK( std::get<ParseError>( result ).line == 4 );
	}

	SECTION( "Invalid escape sequence" )
	{
		Parser doc( "key: \"\\x\"\n" );
		auto result = doc.parse();
		REQUIRE( std::holds_alternative<ParseError>( result ) );
		CHECK( std::get<ParseError>( result ).line == 1 );
	}
}
