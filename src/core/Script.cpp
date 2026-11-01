
#include <stdexcept>
#include "Script.h"

#include <utils/Language.h>

#include <fstream>
#include <sstream>
#include <algorithm>

using namespace huml;

namespace
{
Pronunciations read_pronunciations( const Value &root, const std::string &file )
{
	Pronunciations list;
	const Value &section = root["pronounce"];
	if ( section.is_missing() )
	{
		return list;
	}
	if ( !section.is_dict() )
	{
		throw std::runtime_error( file + ".huml: `pronounce::` is a dict of what is written and how it is said, in quotes" );
	}
	for ( const auto &[written, spoken]: section.as_dict() )
	{
		if ( written.empty() || !spoken.is_string() )
		{
			throw std::runtime_error( file + ".huml: in `pronounce::`, '" + written + "' has to be said as some text, in quotes" );
		}
		list.emplace_back( written, spoken.as_string() );
	}
	return list;
}
}

bool Script::load( std::string lang )
{
	if ( _native_scripts.find( lang ) != _native_scripts.end() )
	{
		// Already loaded
		if ( std::find( _languages.begin(), _languages.end(), lang ) == _languages.end() )
		{
			_languages.push_back( lang ); // and in use again
		}
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
		Value root;
		try
		{
			root = load_recursive( candidate );
		}
		catch ( ... )
		{
			continue;
		}
		_pronunciations[lang] = read_pronunciations( root, candidate ); // (a mistake here is a mistake, not a reason to look further)
		_native_scripts[lang] = root;
		_languages.push_back( lang );
		return true;
	}

	return false;
}

void Script::select_languages( const std::vector<std::string> &languages )
{
	if ( languages.empty() )
	{
		throw std::invalid_argument( "select_languages needs at least one language" );
	}
	for ( const auto &lang: languages )
	{
		if ( _native_scripts.find( lang ) == _native_scripts.end() )
		{
			throw std::invalid_argument( "language not loaded: " + lang );
		}
	}
	_languages = languages;
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

