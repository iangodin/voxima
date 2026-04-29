#include "Utf8.h"
#include <iostream>
#include <string_view>
#include <cassert>

int main()
{
	std::string_view str = "A€𝄞";
	
	try {
		char32_t c1 = decode_utf8( str );
		std::cout << "Decoded: " << (char)c1 << " (remaining: " << str << ")" << std::endl;
		
		char32_t c2 = decode_utf8( str );
		std::cout << "Decoded: " << (char)c2 << " (remaining: " << str << ")" << std::endl;
		
		char32_t c3 = decode_utf8( str );
		std::cout << "Decoded: " << (char)c3 << " (remaining: " << str << ")" << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}

	return 0;
}
