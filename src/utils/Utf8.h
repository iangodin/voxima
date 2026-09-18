
#pragma once

#include <string>
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

inline void append_utf8( std::string &out, char32_t codepoint )
{
	if ( codepoint < 0x80 )
	{
		out += static_cast<char>( codepoint );
	}
	else if ( codepoint < 0x800 )
	{
		out += static_cast<char>( 0xC0 | ( codepoint >> 6 ) );
		out += static_cast<char>( 0x80 | ( codepoint & 0x3F ) );
	}
	else if ( codepoint < 0x10000 )
	{
		out += static_cast<char>( 0xE0 | ( codepoint >> 12 ) );
		out += static_cast<char>( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) );
		out += static_cast<char>( 0x80 | ( codepoint & 0x3F ) );
	}
	else
	{
		out += static_cast<char>( 0xF0 | ( codepoint >> 18 ) );
		out += static_cast<char>( 0x80 | ( ( codepoint >> 12 ) & 0x3F ) );
		out += static_cast<char>( 0x80 | ( ( codepoint >> 6 ) & 0x3F ) );
		out += static_cast<char>( 0x80 | ( codepoint & 0x3F ) );
	}
}
