
#include "StyledText.h"

#include <functional>

namespace
{
	void apply_to_style( IntervalMap<size_t, TextStyle> &map, size_t start, size_t end, std::function<void( TextStyle & )> &&func )
	{
		map.fill_gaps( start, end, TextStyle{} );
		map.apply( start, end, std::move( func ) );
	}
}

std::optional<size_t> StyledText::index( size_t line, size_t column )
{
	if ( line < _lines.size() )
	{
		auto text = _lines.at( line );
		if ( column < text.size() )
		{
			return static_cast<size_t>( std::distance( _utf8.c_str(), text.data() + column ) );
		}
	}

	return std::nullopt;
}

void StyledText::split_lines( void )
{
	_lines.clear();
	// Split the UTF-8 text into lines based on newline characters
	// Lines are stored as string_views pointing into _utf8

	size_t pos = 0;
	while ( pos < _utf8.size() )
	{
		// Find the next newline character
		size_t newline_pos = _utf8.find( '\n', pos );

		if ( newline_pos == std::string::npos )
		{
			// No more newline, add the rest of the text as one final line
			_lines.push_back( std::string_view( _utf8.data() + pos, _utf8.size() - pos ) );
			break;
		}
		else
		{
			// Add text before newline as a line
			_lines.push_back( std::string_view( _utf8.data() + pos, newline_pos - pos ) );

			// Move to after the newline
			pos = newline_pos + 1;
		}
	}
}

void StyledText::set_color( size_t start, size_t end, const Color &color )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.color = color; } );
}

void StyledText::set_font_style( size_t start, size_t end, const FontStyle &style )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.font_style = style; } );
}

void StyledText::set_intent( size_t start, size_t end, std::string_view intent )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.font_style.set_intent( intent ); } );
}

void StyledText::set_weight( size_t start, size_t end, Weight weight )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.font_style.set_weight( weight ); } );
}

void StyledText::set_slant( size_t start, size_t end, Slant slant )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.font_style.set_slant( slant ); } );
}

void StyledText::set_scale( size_t start, size_t end, Scale scale )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.font_style.set_scale( scale ); } );
}

void StyledText::set_width( size_t start, size_t end, Width width )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.font_style.set_width( width ); } );
}

void StyledText::set_typeclass( size_t start, size_t end, TypeClass typeclass )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.font_style.set_typeclass( typeclass ); } );
}

