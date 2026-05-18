
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

	void set_intent( size_t start, size_t end, std::string_view intent );

	void set_weight( size_t start, size_t end, Weight weight );

	void set_slant( size_t start, size_t end, Slant slant );

	void set_scale( size_t start, size_t end, Scale scale );

	void set_width( size_t start, size_t end, Width width );

	void set_typeclass( size_t start, size_t end, TypeClass typeclass );

private:
	void split_lines();

	std::string _utf8;
	IntervalMap<size_t,TextStyle> _style;
	std::vector<std::string_view> _lines;
};

