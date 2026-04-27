
#pragma once

#include <string>
#include <vector>

inline std::string to_base64( const std::vector<unsigned char> &data )
{
	static const char lookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string out;
	out.reserve( ( data.size() + 2 ) / 3 * 4 );

	int val = 0, valb = -6;
	for ( unsigned char c : data )
	{
		val = ( val << 8 ) + c;
		valb += 8;
		while ( valb >= 0 )
		{
			out.push_back( lookup[( val >> valb ) & 0x3F] );
			valb -= 6;
		}
	}
	if ( valb > -6 ) out.push_back( lookup[( ( val << 8 ) >> ( valb + 8 ) ) & 0x3F] );
	while ( out.size() % 4 ) out.push_back( '=' );
	return out;
}
