
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

private:
	void split_lines();

	std::string _utf8;
	IntervalMap<size_t,TextStyle> _font;
	IntervalMap<size_t,Color> _color;
	std::vector<std::string_view> _lines;
};

