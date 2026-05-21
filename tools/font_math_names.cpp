
#include <ft2build.h>
#include <iomanip>
#include <iostream>
#include FT_FREETYPE_H

void dump_font_math_names( const char *font_path )
{
	FT_Library library;
	if ( FT_Init_FreeType( &library ) )
	{
		std::cerr << "Error: Could not initialize FreeType library.\n";
		return;
	}

	FT_Face face;
	if ( FT_New_Face( library, font_path, 0, &face ) )
	{
		std::cerr << "Error: Could not load font file: " << font_path << "\n";
		FT_Done_FreeType( library );
		return;
	}

	// Ensure we are explicitly using the Unicode character map
	if ( FT_Select_Charmap( face, FT_ENCODING_UNICODE ) )
	{
		std::cerr << "Warning: Unicode charmap not found or could not be selected.\n";
	}

	// Check if the font actually supports PostScript glyph names
	if ( !( face->face_flags & FT_FACE_FLAG_GLYPH_NAMES ) )
	{
		std::cerr << "Error: This font format does not store internal glyph names.\n";
		FT_Done_Face( face );
		FT_Done_FreeType( library );
		return;
	}

	std::cout << "==================================================\n";
	std::cout << "Dumping Math-range Glyph Names for: " << face->family_name << "\n";
	std::cout << "==================================================\n";
	std::cout << std::left << std::setw( 12 ) << "Unicode" << std::setw( 12 ) << "Glyph ID"
			  << "PostScript/MATH Name\n";
	std::cout << "--------------------------------------------------\n";

	FT_UInt glyph_index;
	// Start iterating through all defined characters in the charmap
	FT_ULong codepoint = FT_Get_First_Char( face, &glyph_index );

	while ( glyph_index != 0 )
	{
		// Filter specifically for standard mathematical operators & symbols ranges
		// Mathematical Operators: U+2200 to U+22FF
		// Supplemental Math Operators: U+2A00 to U+2AFF
		// Math Alphanumeric Symbols: U+1D400 to U+1D7FF
		bool is_math_range = ( codepoint >= 0x2200 && codepoint <= 0x22FF ) ||
		                     ( codepoint >= 0x2A00 && codepoint <= 0x2AFF ) ||
		                     ( codepoint >= 0x1D400 && codepoint <= 0x1D7FF );

		if ( is_math_range )
		{
			char glyph_name[64];
			// Fetch the naming string mapped to this specific Glyph ID index
			if ( FT_Get_Glyph_Name( face, glyph_index, glyph_name, sizeof( glyph_name ) ) == 0 )
			{
				std::cout << "U+" << std::uppercase << std::hex << std::setw( 6 ) << std::right
						  << std::setfill( '0' ) << codepoint << std::setfill( ' ' ) << "  "
						  << glyph_name << "\n";
			}
		}

		// Advance to the next encoded character index in the file
		codepoint = FT_Get_Next_Char( face, codepoint, &glyph_index );
	}

	FT_Done_Face( face );
	FT_Done_FreeType( library );
}

int main( int argc, char **argv )
{
	if ( argc < 2 )
	{
		std::cout << "Usage: " << argv[0] << " <path_to_math_font.otf>\n";
		return 1;
	}

	dump_font_math_names( argv[1] );
	return 0;
}
