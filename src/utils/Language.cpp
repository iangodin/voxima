
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

std::string container_language( std::string_view tag, bool bibliographic )
{
	struct Codes
	{
		std::string_view two;
		std::string_view terminology;
		std::string_view biblio;
	};
	// What the speech model speaks, and other common ones
	static constexpr Codes codes[] = {
		{ "ar", "ara", "ara" }, { "bn", "ben", "ben" }, { "ca", "cat", "cat" }, { "cs", "ces", "cze" }, { "da", "dan", "dan" },
		{ "de", "deu", "ger" }, { "el", "ell", "gre" }, { "en", "eng", "eng" }, { "es", "spa", "spa" }, { "fa", "fas", "per" },
		{ "fi", "fin", "fin" }, { "fr", "fra", "fre" }, { "he", "heb", "heb" }, { "hi", "hin", "hin" }, { "hu", "hun", "hun" },
		{ "id", "ind", "ind" }, { "it", "ita", "ita" }, { "ja", "jpn", "jpn" }, { "ko", "kor", "kor" }, { "ms", "msa", "may" },
		{ "nb", "nob", "nob" }, { "nl", "nld", "dut" }, { "no", "nor", "nor" }, { "pl", "pol", "pol" }, { "pt", "por", "por" },
		{ "ro", "ron", "rum" }, { "ru", "rus", "rus" }, { "sv", "swe", "swe" }, { "sw", "swa", "swa" }, { "ta", "tam", "tam" },
		{ "th", "tha", "tha" }, { "tr", "tur", "tur" }, { "uk", "ukr", "ukr" }, { "vi", "vie", "vie" }, { "zh", "zho", "chi" },
	};
	const std::string primary = primary_subtag( tag );
	for ( const Codes &code: codes )
	{
		if ( code.two == primary )
		{
			return std::string( bibliographic ? code.biblio : code.terminology );
		}
	}
	return primary;
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
