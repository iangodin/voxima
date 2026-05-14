#include "Script.h"

#include <string_view>
#include <fstream>
#include <sstream>

using namespace huml;

bool Script::load( const std::string &lang )
{
	_impl.reset();
	try
	{
		Value root = load_recursive( lang );
		_impl = std::make_shared<Impl>( std::move( root ) );
		return true;
	}
	catch ( ... )
	{
		return false;
	}
}

Value Script::load_recursive( const std::string &lang )
{
	auto path = _directory / ( lang + ".huml" );
	std::ifstream is( path );
	if ( !is.is_open() )
	{
		throw std::runtime_error( "unable to open file " + path.string() );
	}
	std::stringstream ss;
	ss << is.rdbuf();
	std::string source = ss.str();

	Parser parser( source );
	ParseResult result = parser.parse();
	if ( !std::holds_alternative<Value>( result ) )
	{
		const auto &error = std::get<ParseError>( result );
		throw std::runtime_error(
			path.string() + ": " + error.message + " at line " + std::to_string( error.line ) );
	}

	Value current = std::get<Value>( result );
	const Value &inherit = current["meta"]["inherit"];
	if ( inherit.is_string() )
	{
		Value parent = load_recursive( inherit.as_string() );
		return merge( parent, current );
	}

	return current;
}

Script::Script( Value &&root )
{
	_impl = std::make_shared<Impl>( std::move( root ) );
}
