
#include "Document.h"
#include <iostream>
#include <algorithm>
#include <charconv>
#include <optional>
#include <string_view>
#include <limits>

namespace
{
const huml::Value missing{ huml::Missing{} };
}

namespace huml
{

// Dictionary access sugar
const Value &Value::operator[]( const std::string &key ) const
{
	if ( is_dict() )
	{
		const auto &dict = as_dict();
		auto it = dict.find( key );
		if ( it != dict.end() )
		{
			return it->second;
		}
	}

	return missing;
}

const Value &Value::operator[]( size_t i ) const
{
	if ( is_list() )
	{
		const auto &list = as_list();
		if ( i < list.size() )
		{
			return list[i];
		}
	}

	return missing;
}

size_t Value::size() const
{
	if ( is_dict() )
	{
		return std::get<Dict>( *this ).size();
	}
	else if ( is_list() )
	{
		return std::get<List>( *this ).size();
	}
	else
	{
		return 0;
	}
}

bool Value::empty() const
{
	if ( is_dict() )
	{
		return std::get<Dict>( *this ).empty();
	}
	else if ( is_list() )
	{
		return std::get<List>( *this ).empty();
	}
	else
	{
		return true;
	}
}

class Parser
{
public:
	explicit Parser( std::string_view source ) :
		view_( source )
	{
	}

	ParseResult parse()
	{
		skip_initial( view_ );
		auto root = parse_root( view_ );
		if ( root )
		{
			return std::move( *root );
		}
		if ( last_error_ )
		{
			return *last_error_;
		}
		return ParseError{ "Unknown error", line_ };
	}

private:
	std::string_view view_;
	size_t line_ = 1;
	std::optional<ParseError> last_error_;

	void advance( std::string_view &view, size_t n )
	{
		for ( size_t i = 0; i < n; ++i )
		{
			if ( view[i] == '\n' )
			{
				line_++;
			}
		}
		view.remove_prefix( n );
	}

	void set_error( std::string message )
	{
		if ( !last_error_ )
		{
			last_error_ = ParseError{ std::move( message ), line_ };
		}
	}

	void skip_to_next_content( std::string_view &view )
	{
		while ( !view.empty() )
		{
			size_t i = 0;
			while ( i < view.size() && view[i] == ' ' )
				i++;

			if ( i == view.size() )
			{
				advance( view, i );
				break;
			}

			if ( view[i] == '\n' )
			{
				advance( view, i + 1 );
				continue;
			}

			if ( view[i] == '#' )
			{
				consume_line( view );
				continue;
			}

			break;
		}
	}

	void skip_initial( std::string_view &view )
	{
		if ( view.starts_with( "%HUML" ) )
		{
			consume_line( view );
		}
		skip_to_next_content( view );
	}

	void consume_line( std::string_view &view )
	{
		auto pos = view.find( '\n' );
		if ( pos == std::string_view::npos )
		{
			advance( view, view.size() );
		}
		else
		{
			advance( view, pos + 1 );
		}
	}

	std::optional<Value> parse_root( std::string_view &view )
	{
		if ( view.empty() )
			return Value{ std::monostate{} };

		return parse_indented_value( view, 0, true );
	}

	std::optional<Value> parse_scalar_or_inline( std::string_view &view )
	{
		size_t spaces = 0;
		while ( spaces < view.size() && view[spaces] == ' ' )
			spaces++;
		advance( view, spaces );

		if ( view.empty() )
		{
			set_error( "Unexpected end of input" );
			return std::nullopt;
		}

		if ( view.starts_with( "[]" ) )
		{
			advance( view, 2 );
			return Value{ List{} };
		}
		if ( view.starts_with( "{}" ) )
		{
			advance( view, 2 );
			return Value{ Dict{} };
		}

		if ( view.starts_with( '"' ) )
		{
			auto s = parse_quoted_string( view );
			if ( !s )
				return std::nullopt;
			return Value{ std::move( *s ) };
		}
		if ( view.starts_with( "true" ) )
		{
			advance( view, 4 );
			return Value{ true };
		}
		if ( view.starts_with( "false" ) )
		{
			advance( view, 5 );
			return Value{ false };
		}
		if ( view.starts_with( "null" ) )
		{
			advance( view, 4 );
			return Value{ std::monostate{} };
		}

		if ( view.starts_with( "nan" ) )
		{
			advance( view, 3 );
			return Value{ std::numeric_limits<double>::quiet_NaN() };
		}
		if ( view.starts_with( "+inf" ) )
		{
			advance( view, 4 );
			return Value{ std::numeric_limits<double>::infinity() };
		}
		if ( view.starts_with( "-inf" ) )
		{
			advance( view, 4 );
			return Value{ -std::numeric_limits<double>::infinity() };
		}
		if ( view.starts_with( "inf" ) )
		{
			advance( view, 3 );
			return Value{ std::numeric_limits<double>::infinity() };
		}

		return parse_number( view );
	}

	std::optional<Value> parse_number( std::string_view &view )
	{
		auto end = view.find_first_of( ", \n]}#" );
		std::string_view sub = ( end == std::string_view::npos ) ? view
																 : view.substr( 0, end );

		std::string s( sub );
		s.erase( std::remove( s.begin(), s.end(), '_' ), s.end() );

		std::string_view sv = s;
		int64_t sign = 1;
		if ( sv.starts_with( '+' ) )
		{
			sv.remove_prefix( 1 );
		}
		else if ( sv.starts_with( '-' ) )
		{
			sv.remove_prefix( 1 );
			sign = -1;
		}

		int base = 10;
		if ( sv.starts_with( "0x" ) )
		{
			base = 16;
			sv.remove_prefix( 2 );
		}
		else if ( sv.starts_with( "0o" ) )
		{
			base = 8;
			sv.remove_prefix( 2 );
		}
		else if ( sv.starts_with( "0b" ) )
		{
			base = 2;
			sv.remove_prefix( 2 );
		}

		advance( view, sub.size() );

		if ( base == 10 &&
			 ( sv.find( '.' ) != std::string_view::npos ||
			   sv.find( 'e' ) != std::string_view::npos ) )
		{
			try
			{
				size_t processed;
				double val = std::stod( s, &processed );
				return Value{ val };
			}
			catch ( ... )
			{
				return Value{ std::string( sub ) };
			}
		}

		try
		{
			int64_t result = 0;
			auto [ptr, ec] =
				std::from_chars( sv.data(), sv.data() + sv.size(), result, base );

			if ( ec == std::errc{} )
			{
				return Value{ sign * result };
			}
			else
			{
				return Value{ std::string( sub ) };
			}
		}
		catch ( ... )
		{
			return Value{ std::string( sub ) };
		}
	}

	std::optional<List> parse_multiline_list( std::string_view &view, int indent )
	{
		List list;
		while ( !view.empty() )
		{
			if ( !check_indent( view, indent ) )
				break;

			advance( view, static_cast<size_t>( indent ) * 2 );
			if ( !view.starts_with( "- " ) )
			{
				// This shouldn't happen if is_list_item checked out, but we should be safe
				break;
			}

			advance( view, 2 ); // consume "- "
			auto val = parse_indented_value( view, indent + 1, true );
			if ( !val )
				return std::nullopt;
			list.push_back( std::move( *val ) );
			skip_to_next_content( view );
		}
		return list;
	}

	std::optional<Dict> parse_multiline_dict( std::string_view &view, int indent )
	{
		Dict dict;
		while ( !view.empty() )
		{
			if ( !check_indent( view, indent ) )
				break;

			advance( view, static_cast<size_t>( indent ) * 2 );
			auto key = parse_key( view );
			if ( !key )
			{
				set_error( "Failed to parse key" );
				return std::nullopt;
			}

			bool is_vector = false;
			if ( view.starts_with( "::" ) )
			{
				is_vector = true;
				advance( view, 2 );
			}
			else if ( view.starts_with( ":" ) )
			{
				advance( view, 1 );
			}
			else
			{
				set_error( "Expected ':' after key" );
				return std::nullopt;
			}

			bool has_inline_content = false;
			if ( view.starts_with( ' ' ) )
			{
				size_t i = 1;
				while ( i < view.size() && view[i] == ' ' )
					i++;
				if ( i < view.size() && view[i] != '\n' && view[i] != '#' )
					has_inline_content = true;

				if ( has_inline_content )
					advance( view, 1 );
			}

			if ( is_vector )
			{
				if ( has_inline_content )
				{
					auto v = parse_inline_vector( view );
					if ( !v )
						return std::nullopt;
					dict[std::move( *key )] = std::move( *v );
				}
				else
				{
					auto v = parse_indented_value( view, indent + 1, true );
					if ( !v )
						return std::nullopt;
					dict[std::move( *key )] = std::move( *v );
				}
			}
			else
			{
				auto v = parse_indented_value( view, indent + 1, false );
				if ( !v )
					return std::nullopt;
				dict[std::move( *key )] = std::move( *v );
			}
			skip_to_next_content( view );
		}
		return dict;
	}

	std::optional<Value> parse_inline_vector( std::string_view &view )
	{
		size_t spaces = 0;
		while ( spaces < view.size() && view[spaces] == ' ' )
			spaces++;
		advance( view, spaces );

		if ( view.starts_with( "[]" ) )
		{
			advance( view, 2 );
			return Value{ List{} };
		}
		if ( view.starts_with( "{}" ) )
		{
			advance( view, 2 );
			return Value{ Dict{} };
		}

		std::string_view temp = view;
		auto key_test = parse_key( temp );
		if ( key_test && temp.starts_with( ':' ) )
		{
			Dict dict;
			while ( !view.empty() )
			{
				auto key = parse_key( view );
				if ( !key )
					break;
				if ( !view.starts_with( ':' ) )
					break;
				advance( view, 1 );
				if ( view.starts_with( ' ' ) )
					advance( view, 1 );
				auto val = parse_scalar_or_inline( view );
				if ( !val )
					return std::nullopt;
				dict[std::move( *key )] = std::move( *val );

				if ( view.starts_with( ',' ) )
				{
					advance( view, 1 );
					if ( view.starts_with( ' ' ) )
						advance( view, 1 );
				}
				else
				{
					break;
				}
			}
			return Value{ std::move( dict ) };
		}
		else
		{
			List list;
			while ( !view.empty() )
			{
				auto val = parse_scalar_or_inline( view );
				if ( !val )
					return std::nullopt;
				list.push_back( std::move( *val ) );
				if ( view.starts_with( ',' ) )
				{
					advance( view, 1 );
					if ( view.starts_with( ' ' ) )
						advance( view, 1 );
				}
				else
				{
					break;
				}
			}
			return Value{ std::move( list ) };
		}
	}

	std::optional<Value> parse_indented_value(
		std::string_view &view, int expected_indent, bool allow_multiline_vector )
	{
		size_t spaces = 0;
		while ( spaces < view.size() && view[spaces] == ' ' )
			spaces++;
		advance( view, spaces );

		if ( view.starts_with( '#' ) )
			consume_line( view );

		skip_to_next_content( view );

		if ( allow_multiline_vector && view.starts_with( "::" ) )
		{
			advance( view, 2 );
			bool has_inline_content = false;
			if ( view.starts_with( ' ' ) )
			{
				size_t i = 1;
				while ( i < view.size() && view[i] == ' ' )
					i++;
				if ( i < view.size() && view[i] != '\n' && view[i] != '#' )
					has_inline_content = true;

				if ( has_inline_content )
					advance( view, 1 );
			}

			if ( has_inline_content )
				return parse_inline_vector( view );

			return parse_indented_value( view, expected_indent, true );
		}

		if ( view.starts_with( "\"\"\"" ) )
		{
			auto ms = parse_multiline_string( view, expected_indent );
			if ( !ms )
				return std::nullopt;
			return Value{ std::move( *ms ) };
		}

		if ( allow_multiline_vector && check_indent( view, expected_indent ) )
		{
			std::string_view temp = view;
			temp.remove_prefix( static_cast<size_t>( expected_indent ) * 2 );
			if ( temp.starts_with( "- " ) )
			{
				auto l = parse_multiline_list( view, expected_indent );
				if ( !l )
					return std::nullopt;
				return Value{ std::move( *l ) };
			}
			if ( is_key( temp ) )
			{
				auto d = parse_multiline_dict( view, expected_indent );
				if ( !d )
					return std::nullopt;
				return Value{ std::move( *d ) };
			}
		}

		return parse_scalar_or_inline( view );
	}

	std::optional<MultiLine> parse_multiline_string( std::string_view &view, int indent )
	{
		advance( view, 3 );
		consume_line( view );

		MultiLine lines;
		size_t base_spaces = static_cast<size_t>( indent ) * 2;

		while ( !view.empty() )
		{
			std::string_view temp = view;
			size_t spaces = 0;
			while ( temp.starts_with( ' ' ) )
			{
				temp.remove_prefix( 1 );
				spaces++;
			}
			if ( temp.starts_with( "\"\"\"" ) && spaces == static_cast<size_t>( indent - 1 ) * 2 )
			{
				advance( view, spaces + 3 );
				break;
			}

			auto eol = view.find( '\n' );
			std::string_view line = view.substr( 0, eol );
			if ( line.size() >= base_spaces )
				line.remove_prefix( base_spaces );
			lines.push_back( std::string( line ) );

			if ( eol == std::string_view::npos )
			{
				advance( view, view.size() );
			}
			else
			{
				advance( view, eol + 1 );
			}
		}
		return lines;
	}

	std::optional<std::string> parse_key( std::string_view &view )
	{
		if ( view.starts_with( '"' ) )
			return parse_quoted_string( view );
		auto eol = view.find( '\n' );
		auto colon = view.find( ':' );
		if ( colon == std::string_view::npos ||
			 ( eol != std::string_view::npos && colon > eol ) )
			return std::nullopt;
		std::string_view key_view = view.substr( 0, colon );
		while ( !key_view.empty() && key_view.back() == ' ' )
			key_view = key_view.substr( 0, key_view.size() - 1 );
		std::string key( key_view );
		advance( view, colon );
		return key;
	}

	bool is_key( std::string_view v )
	{
		if ( v.empty() )
			return false;
		size_t i = 0;
		while ( i < v.size() && v[i] == ' ' )
			i++;
		if ( i == v.size() )
			return false;
		return ( v[i] >= 'a' && v[i] <= 'z' ) || ( v[i] >= 'A' && v[i] <= 'Z' ) ||
			v[i] == '"';
	}

	bool check_indent( std::string_view view, int level )
	{
		size_t spaces = static_cast<size_t>( level ) * 2;
		if ( view.size() < spaces )
			return false;
		for ( size_t i = 0; i < spaces; ++i )
			if ( view[i] != ' ' )
				return false;
		if ( view.size() > spaces && view[spaces] == ' ' )
			return false;
		return true;
	}

	std::optional<std::string> parse_quoted_string( std::string_view &view )
	{
		advance( view, 1 );
		std::string result;
		while ( !view.empty() )
		{
			if ( view[0] == '\\' )
			{
				if ( view.size() < 2 )
				{
					set_error( "Unexpected end of input in escape sequence" );
					return std::nullopt;
				}
				switch ( view[1] )
				{
					case '"': result += '"'; break;
					case '\\': result += '\\'; break;
					case 'b': result += '\b'; break;
					case 'f': result += '\f'; break;
					case 'n': result += '\n'; break;
					case 'r': result += '\r'; break;
					case 't': result += '\t'; break;
					case 'v': result += '\v'; break;
					default:
						set_error( "Invalid escape sequence" );
						return std::nullopt;
				}
				advance( view, 2 );
			}
			else if ( view[0] == '"' )
			{
				advance( view, 1 );
				return result;
			}
			else
			{
				result += view[0];
				advance( view, 1 );
			}
		}
		set_error( "Unclosed quoted string" );
		return std::nullopt;
	}
};

ParseResult Document::parse( std::string source )
{
	_impl.reset();

	std::string_view view = source;
	Parser parser( view );
	ParseResult result = parser.parse();
	if ( std::holds_alternative<Value>( result ) )
	{
		_impl = std::make_shared<Impl>( std::move( source ), std::get<Value>( result ) );
	}

	return result;
}

} // namespace huml
