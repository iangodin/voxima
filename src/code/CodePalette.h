#pragma once

#include <fonts/FontStyle.h>
#include <visuals/Color.h>

#include <map>
#include <optional>
#include <string>
#include <string_view>

// How a kind of code looks: its color, and bold or italic if it is set apart that way
struct CodeStyle
{
	Color color;
	std::optional<Weight> weight = {};
	std::optional<Slant> slant = {};
};

// The colors of syntax highlighting: a style for each kind of code the highlighter names (see
// code::highlight: "keyword", "string", "function" ...), and one for plain code. It is part of a Palette (the
// theme's: Theme::code_palette), worked out from the palette's accents (derive_code_palette) and adjusted there.
class CodePalette
{
public:
	CodeStyle plain;

	// The band behind the lines being pointed out (see LineHighlight); see-through, so the text shows over it
	Color line_highlight = { 1.F, 1.F, 1.F, 0.12F };

	void set( std::string name, CodeStyle style )
	{
		_styles[std::move( name )] = style;
	}

	// The style for a kind: its own, or else that of the kind it is a kind of ("function.special" is a
	// "function"), or else plain
	const CodeStyle &style_for( std::string_view name ) const
	{
		while ( true )
		{
			if ( auto it = _styles.find( name ); it != _styles.end() )
			{
				return it->second;
			}
			const size_t dot = name.rfind( '.' );
			if ( dot == std::string_view::npos )
			{
				return plain;
			}
			name = name.substr( 0, dot );
		}
	}

private:
	std::map<std::string, CodeStyle, std::less<>> _styles;
};
