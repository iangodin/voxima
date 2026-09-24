
#include "Script.h"

#include <utils/Language.h>

#include <fstream>
#include <sstream>
#include <algorithm>

using namespace huml;

bool Script::load( std::string lang )
{
	auto it = std::find( _languages.begin(), _languages.end(), lang );
	if ( it != _languages.end() )
	{
		// Already loaded
		return true;
	}

	// Try the exact requested tag first, then progressively less specific
	// BCP-47 fallbacks (e.g. "en-CA" -> "en") when no file exists for the
	// exact tag. Whichever file actually supplied the content, the result is
	// still stored under the tag the caller asked for, so later lookups
	// (fetch(), TTS language derivation, subtitle/track metadata) stay keyed
	// on what was requested rather than silently rewritten.
	for ( const auto &candidate: language::fallback_chain( lang ) )
	{
		try
		{
			Value root = load_recursive( candidate );
			_native_scripts[lang] = root;
			_languages.push_back( lang );
			return true;
		}
		catch ( ... )
		{
			continue;
		}
	}

	return false;
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

