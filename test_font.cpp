#include "Theme.h"
#include "NewComputerModern.h"
#include "TextStyle.h"
#include <iostream>

int main()
{
	try
	{
		Theme theme = NewComputerModern::create_theme();
		TextStyle style( "prose" );
		style.large().serif();

		FontStack stack = theme.get_stack( style );
		auto entry = stack.lookup_codepoint( U'A' );

		std::cout << "Found face for 'A' with glyph index " << entry.glyph_index 
		          << std::endl;
	}
	catch ( const std::exception &e )
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
