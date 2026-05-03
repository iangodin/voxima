
#include "Script.h"

#include <string_view>

using namespace huml;

bool Script::parse( std::string_view source )
{
	_impl.reset();

	Parser parser( source );
	ParseResult result = parser.parse();
	if ( std::holds_alternative<Value>( result ) )
	{
		_impl = std::make_shared<Impl>( std::get<Value>( result ) );
	}
	else
	{
		const auto &error = std::get<ParseError>( result );
		throw std::runtime_error(
			error.message + " at line " + std::to_string( error.line ) );
	}

	return _impl != nullptr;
}

Script::Script( Value &&root )
{
	_impl = std::make_shared<Impl>( std::move( root ) );
}

