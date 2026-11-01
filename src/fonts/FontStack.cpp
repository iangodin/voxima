#include "fonts/FontStack.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <stdexcept>
#include <string>

std::optional<GlyphEntry> FontStack::find_codepoint( char32_t codepoint ) const
{
	for ( size_t font_index = 0; font_index < _stack.size(); ++font_index )
	{
		auto &entry = _stack.at( font_index );
		FT_UInt glyph_index = FT_Get_Char_Index( entry.face.get(), codepoint );
		if ( glyph_index != 0 )
		{
			return GlyphEntry{ font_index, glyph_index };
		}
	}
	return std::nullopt;
}

GlyphEntry FontStack::lookup_codepoint( char32_t codepoint ) const
{
	if ( auto entry = find_codepoint( codepoint ) )
	{
		return *entry;
	}
	throw std::runtime_error( "Codepoint not found in font stack: " + std::to_string( uint32_t( codepoint ) ) );
}
