
#include "Language.h"

#include <algorithm>
#include <cctype>

namespace language
{

namespace
{

std::string to_lower( std::string_view s )
{
	std::string result( s );
	std::transform(
		result.begin(), result.end(), result.begin(),
		[]( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
	return result;
}

}

std::string primary_subtag( std::string_view tag )
{
	auto dash = tag.find( '-' );
	std::string_view primary = ( dash == std::string_view::npos ) ? tag : tag.substr( 0, dash );
	return to_lower( primary );
}

std::vector<std::string> fallback_chain( std::string_view tag )
{
	std::vector<std::string> chain;
	chain.emplace_back( tag );

	if ( tag.find( '-' ) != std::string_view::npos )
	{
		std::string primary = primary_subtag( tag );
		if ( primary != tag )
		{
			chain.push_back( std::move( primary ) );
		}
	}

	return chain;
}

}
