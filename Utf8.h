#pragma once

#include <string_view>
#include <stdexcept>

inline char32_t decode_utf8( std::string_view &str )
{
	if ( str.empty() )
	{
		throw std::runtime_error( "Empty string" );
	}

	unsigned char first = static_cast<unsigned char>( str[0] );
	char32_t codepoint = 0;
	size_t length = 0;

	if ( ( first & 0x80 ) == 0 )
	{
		codepoint = first;
		length = 1;
	}
	else if ( ( first & 0xE0 ) == 0xC0 )
	{
		if ( str.size() < 2 ) throw std::runtime_error( "Invalid UTF-8" );
		codepoint = ( first & 0x1F ) << 6 | ( static_cast<unsigned char>( str[1] ) & 0x3F );
		length = 2;
	}
	else if ( ( first & 0xF0 ) == 0xE0 )
	{
		if ( str.size() < 3 ) throw std::runtime_error( "Invalid UTF-8" );
		codepoint = ( first & 0x0F ) << 12 | ( static_cast<unsigned char>( str[1] ) & 0x3F ) << 6 | ( static_cast<unsigned char>( str[2] ) & 0x3F );
		length = 3;
	}
	else if ( ( first & 0xF8 ) == 0xF0 )
	{
		if ( str.size() < 4 ) throw std::runtime_error( "Invalid UTF-8" );
		codepoint = ( first & 0x07 ) << 18 | ( static_cast<unsigned char>( str[1] ) & 0x3F ) << 12 | ( static_cast<unsigned char>( str[2] ) & 0x3F ) << 6 | ( static_cast<unsigned char>( str[3] ) & 0x3F );
		length = 4;
	}
	else
	{
		throw std::runtime_error( "Invalid UTF-8" );
	}

	str.remove_prefix( length );
	return codepoint;
}
