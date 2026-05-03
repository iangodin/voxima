#include "fonts/FontStack.h"
#include <stdexcept>

GlyphEntry FontStack::lookup_codepoint( char32_t codepoint ) const
{
	for ( size_t font_index = 0; font_index < _stack.size(); ++font_index )
	{
		auto &entry = _stack.at( font_index );
		FT_UInt glyph_index = FT_Get_Char_Index( entry.face.get(), codepoint );
		if ( glyph_index != 0 )
		{
			return { font_index, glyph_index };
		}
	}
	throw std::runtime_error( "Codepoint not found in font stack: " + std::to_string( uint32_t( codepoint ) ) );
}
