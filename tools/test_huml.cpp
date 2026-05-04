
#include <utils/Huml.h>

#include <filesystem>
#include <iostream>
#include <fstream>

std::string read_file( std::filesystem::path path )
{
	std::size_t size = std::filesystem::file_size( path );
	std::ifstream file( path, std::ios::binary );
	std::string buffer( size, '\0' );
	file.read( buffer.data(), std::streamsize( size ) );
	return buffer;
}

int main( int argc, char *argv[] )
{
	if ( argc != 2 )
	{
		std::cerr << "Usage: test_huml <huml-file>\n";
		return 1;
	}

	std::string source = read_file( argv[1] );

	huml::Parser parser( source );
	auto result = parser.parse();
	std::cout << result << '\n';

	return 0;
}
