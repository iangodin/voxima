
#include "StyledText.h"

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
	TextStyle style = {};
	style.color = color;
	_style.fill_gaps( start, end, style );
	_style.apply( start, end, [&]( TextStyle &sty )
	{
		sty.color = color;
	} );
}

void StyledText::set_font_style( size_t start, size_t end, const FontStyle &style )
{
	TextStyle tstyle = {};
	tstyle.font_style = style;
	_style.fill_gaps( start, end, tstyle );
	_style.apply( start, end, [&]( TextStyle &sty )
	{
		sty.font_style = style;
	} );
}

void StyledText::set_intent( size_t start, size_t end, std::string_view intent )
{
	TextStyle style = {};
	style.font_style._intent = std::string( intent );
	_style.fill_gaps( start, end, style );
	_style.apply( start, end, [&]( TextStyle &sty )
	{
		sty.font_style._intent = intent;
	} );
}

void StyledText::set_weight( size_t start, size_t end, Weight weight )
{
	TextStyle style = {};
	style.font_style._weight = weight;
	_style.fill_gaps( start, end, style );
	_style.apply( start, end, [&]( TextStyle &sty )
	{
		sty.font_style._weight = weight;
	} );
}

void StyledText::set_slant( size_t start, size_t end, Slant slant )
{
	TextStyle style = {};
	style.font_style._slant = slant;
	_style.fill_gaps( start, end, style );
	_style.apply( start, end, [&]( TextStyle &sty )
	{
		sty.font_style._slant = slant;
	} );
}

void StyledText::set_scale( size_t start, size_t end, Scale scale )
{
	TextStyle style = {};
	style.font_style._scale = scale;
	_style.fill_gaps( start, end, style );
	_style.apply( start, end, [&]( TextStyle &sty )
	{
		sty.font_style._scale = scale;
	} );
}

void StyledText::set_width( size_t start, size_t end, Width width )
{
	TextStyle style = {};
	style.font_style._width = width;
	_style.fill_gaps( start, end, style );
	_style.apply( start, end, [&]( TextStyle &sty )
	{
		sty.font_style._width = width;
	} );
}

void StyledText::set_typeclass( size_t start, size_t end, TypeClass typeclass )
{
	TextStyle style = {};
	style.font_style._typeclass = typeclass;
	_style.fill_gaps( start, end, style );
	_style.apply( start, end, [&]( TextStyle &sty )
	{
		sty.font_style._typeclass = typeclass;
	} );
}

