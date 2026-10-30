
#pragma once

#include "TextStyle.h"

#include <utils/IntervalMap.h>

#include <optional>

class StyledText
{
public:
	StyledText() = default;

	StyledText( const StyledText & ) = default;
	StyledText( StyledText && ) = default;

	~StyledText() = default;

	StyledText &operator=( const StyledText & ) = default;
	StyledText &operator=( StyledText && ) = default;

	StyledText( std::string text ) :
		_utf8( std::move( text ) )
	{
		split_lines();
	}

	size_t line_count( void ) const
	{
		return _lines.size();
	}

	size_t size( void ) const
	{
		return _utf8.size();
	}

	std::string_view line( size_t index )
	{
		return _lines.at( index );
	}

	std::optional<size_t> index( size_t line, size_t column );

	const std::vector<std::string_view> &lines( void ) const
	{
		return _lines;
	}

	void set_color( size_t start, size_t end, const Color &color );

	void set_font_style( size_t start, size_t end, const FontStyle &style );

	void set_intent( size_t start, size_t end, std::string_view intent );

	void set_weight( size_t start, size_t end, Weight weight );

	void set_slant( size_t start, size_t end, Slant slant );

	void set_scale( size_t start, size_t end, Scale scale );

	void set_width( size_t start, size_t end, Width width );

	void set_typeclass( size_t start, size_t end, TypeClass typeclass );

	void set_bidi_level( size_t start, size_t end, size_t level );

	// BCP-47 tag (e.g. "en", "fr-CA") passed to Harfbuzz for this range, in
	// addition to the script/direction hb_buffer_guess_segment_properties
	// already infers from the Unicode content itself.
	void set_language( size_t start, size_t end, std::string language );

	void analyze_bidi( void );

	const std::string &text( void ) const
	{
		return _utf8;
	}

	const IntervalMap<size_t, TextStyle> &styles( void ) const
	{
		return _style;
	}

private:
	void split_lines();

	std::string _utf8;
	IntervalMap<size_t,TextStyle> _style;
	std::vector<std::string_view> _lines;
};

