
#include <catch2/catch_test_macros.hpp>

#include <utils/Huml.h>

#include <variant>

using namespace huml;

template <typename... T>
std::ostream &operator<<( std::ostream &os, const std::variant<T...> &v )
{
	std::visit( [&os]( auto &&arg ) { os << arg; }, v );
	return os;
}

TEST_CASE( "HUML parser tests", "[huml]" )
{
	SECTION( "Valid simple document" )
	{
		Parser doc( R"huml(key: "value")huml" );
		auto result = doc.parse();
		CAPTURE( result );
		CHECK( std::holds_alternative<Value>( result ) );
	}

	SECTION( "Invalid quoted string reports correct line" )
	{
		Parser doc( "key: \"unclosed\n" );
		auto result = doc.parse();
		CAPTURE( result );
		REQUIRE( std::holds_alternative<ParseError>( result ) );
		CHECK( std::get<ParseError>( result ).line == 1 );
	}

	SECTION( "Error on later lines" )
	{
		Parser doc( "first: 1\nsecond: 2\nthird: \"unclosed\n" );
		auto result = doc.parse();
		CAPTURE( result );
		REQUIRE( std::holds_alternative<ParseError>( result ) );
		CHECK( std::get<ParseError>( result ).line == 3 );
	}

	SECTION( "Invalid escape sequence" )
	{
		Parser doc( "key: \"\\x\"\n" );
		auto result = doc.parse();
		CAPTURE( result );
		REQUIRE( std::holds_alternative<ParseError>( result ) );
		CHECK( std::get<ParseError>( result ).line == 1 );
	}

	SECTION( "Multiple top-level items" )
	{
		Parser doc( "first: 1\nsecond: 2\n" );
		auto result = doc.parse();
		CAPTURE( result );
		REQUIRE( std::holds_alternative<Value>( result ) );
		const auto &root = std::get<Value>( result );
		REQUIRE( root.is_dict() );
		const auto &dict = root.as_dict();
		CHECK( dict.count( "first" ) == 1 );
		CHECK( dict.count( "second" ) == 1 );
	}

	SECTION( "Bad list using single colon" )
	{
		Parser doc( R"huml(colors:
  - "red"
  - "green"
  - "blue"
)huml" );
		auto result = doc.parse();
		CAPTURE( result );
		REQUIRE( std::holds_alternative<ParseError>( result ) );
		CHECK( std::get<ParseError>( result ).line == 1 );
	}
}
