#include "StyledText.h"
#include <utils/Utf8.h>

#include <fribidi.h>

#include <functional>
#include <vector>

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
	size_t line_index = 0;
	while ( pos < _utf8.size() )
	{
		// Find the next newline character
		size_t newline_pos = _utf8.find( '\n', pos );

		if ( newline_pos == std::string::npos )
		{
			// No more newline, add the rest of the text as one final line
			_lines.push_back( std::string_view( _utf8.data() + pos, _utf8.size() - pos ) );
			apply_to_style( _style, pos, _utf8.size(), [&]( TextStyle &sty ) { sty.line = line_index; } );
			break;
		}
		else
		{
			// Add text before newline as a line
			_lines.push_back( std::string_view( _utf8.data() + pos, newline_pos - pos ) );
			apply_to_style( _style, pos, newline_pos + 1, [&]( TextStyle &sty ) { sty.line = line_index; } );

			// Move to after the newline
			pos = newline_pos + 1;
			line_index++;
		}
	}
	analyze_bidi();
}

void StyledText::analyze_bidi( void )
{
	if ( _utf8.empty() )
	{
		return;
	}

	std::vector<FriBidiChar> fribidi_text;
	std::vector<size_t> utf8_to_utf32;
	std::string_view remaining = _utf8;
	while ( !remaining.empty() )
	{
		size_t pos = _utf8.size() - remaining.size();
		utf8_to_utf32.push_back( pos );
		fribidi_text.push_back( static_cast<FriBidiChar>( decode_utf8( remaining ) ) );
	}
	utf8_to_utf32.push_back( _utf8.size() );

	std::vector<FriBidiCharType> bidi_types( fribidi_text.size() );
	fribidi_get_bidi_types(
		fribidi_text.data(),
		static_cast<FriBidiStrIndex>( fribidi_text.size() ),
		bidi_types.data() );

	FriBidiParType base_dir = FRIBIDI_PAR_ON;
	std::vector<FriBidiLevel> levels( fribidi_text.size() );
	[[maybe_unused]] auto result = fribidi_get_par_embedding_levels(
		bidi_types.data(),
		static_cast<FriBidiStrIndex>( fribidi_text.size() ),
		&base_dir,
		levels.data() );

	for ( size_t i = 0; i < levels.size(); ++i )
	{
		set_bidi_level( utf8_to_utf32[i], utf8_to_utf32[i + 1], static_cast<size_t>( levels[i] ) );
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

void StyledText::set_bidi_level( size_t start, size_t end, size_t level )
{
	apply_to_style( _style, start, end, [&]( TextStyle &sty ) { sty.bidi_level = level; } );
}
