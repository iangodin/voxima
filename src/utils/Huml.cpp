
#include "Huml.h"

#include <limits>
#include <charconv>

namespace
{
constexpr huml::Value missing{ huml::Missing{} };
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

void print( std::ostream &os, const Null &, const std::string &indent )
{
	os << indent << "null";
}

void print( std::ostream &os, const Missing &, const std::string &indent )
{
	os << indent << "missing";
}

void print( std::ostream &os, bool value, const std::string &indent )
{
	if ( value )
	{
		os << indent << "true";
	}
	else
	{
		os << indent << "false";
	}
}

void print( std::ostream &os, int64_t value, const std::string &indent )
{
	os << indent << value;
}

void print( std::ostream &os, double value, const std::string &indent )
{
	os << indent << value;
}

void print( std::ostream &os, const std::string &value, const std::string &indent )
{
	os << indent << '"' << value << '"';
}

void print( std::ostream &os, const MultiLine &lines, const std::string &indent )
{
	os << "\"\"\"\n";
	for ( const auto &line: lines )
	{
		os << indent << "  " << line << '\n';
	}
	os << indent << "\"\"\"\n";
}

void print( std::ostream &os, const List &list, const std::string &indent )
{
	std::string child = indent + "  ";
	os << indent << "[\n";
	for ( const auto &value: list )
	{
		print( os, value, child );
		os << '\n';
	}
	os << indent << "]";
}

void print( std::ostream &os, const Dict &dict, const std::string &indent )
{
	std::string child = indent + "  ";
	os << indent << "{\n";
	for ( const auto &[key, value]: dict )
	{
		os << child << key << ": ";
		if ( value.is_dict() || value.is_list() )
		{
			os << '\n';
			print( os, value, child + "  " );
		}
		else
		{
			print( os, value, "" );
		}
		os << '\n';
	}
	os << indent << "}";
}

void print( std::ostream &output, const Value &value, const std::string &indent )
{
	std::visit( [&]( const auto &v ) { print( output, v, indent ); }, value );
}

ParseResult Parser::parse()
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

void Parser::advance( std::string_view &view, size_t n )
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

// Validates that the remainder of the current line (up to \n or end) has no
// trailing spaces and, if it starts with #, that the comment is well-formed.
// Returns false and sets an error on violation.
bool Parser::check_end_of_line( std::string_view view )
{
	// Find the end of the current logical line.
	auto eol = view.find( '\n' );
	std::string_view line = view.substr( 0, eol );

	// Strip a valid inline comment from the right, if present.
	auto hash = line.find( '#' );
	if ( hash != std::string_view::npos )
	{
		// Preceding the # there must be spaces only (already consumed value).
		// The # must be followed by exactly one space (then comment text).
		std::string_view before_hash = line.substr( 0, hash );
		for ( char c: before_hash )
		{
			if ( c != ' ' )
			{
				set_error( "Unexpected character before inline comment '#'" );
				return false;
			}
		}
		std::string_view comment_body = line.substr( hash ); // includes '#'
		if ( comment_body.size() >= 2 && comment_body[1] != ' ' )
		{
			set_error( "Comment marker '#' must be followed by exactly one space" );
			return false;
		}
		// Check for trailing spaces inside the comment body.
		if ( !comment_body.empty() && comment_body.back() == ' ' )
		{
			set_error( "Trailing spaces are not allowed" );
			return false;
		}
		// Trim to before the comment for trailing-space check below.
		line = before_hash;
	}

	// No trailing spaces allowed on the remaining line content.
	if ( !line.empty() && line.back() == ' ' )
	{
		set_error( "Trailing spaces are not allowed" );
		return false;
	}

	return true;
}

void Parser::skip_to_next_content( std::string_view &view )
{
	while ( !view.empty() )
	{
		size_t i = 0;
		while ( i < view.size() && view[i] == ' ' )
			i++;

		if ( i == view.size() )
		{
			// Blank line (spaces only) — trailing spaces are not allowed.
			if ( i > 0 )
			{
				set_error( "Trailing spaces are not allowed on blank lines" );
				// Advance past to avoid an infinite loop; error is already set.
				advance( view, i );
				return;
			}
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
			// The '#' must be followed by a space (or be the only thing on the line).
			if ( i + 1 < view.size() && view[i + 1] != ' ' && view[i + 1] != '\n' )
			{
				set_error( "Comment marker '#' must be followed by exactly one space" );
				return;
			}
			// Check for trailing spaces inside the comment.
			auto eol = view.find( '\n', i );
			std::string_view comment_line = view.substr(
				i, eol == std::string_view::npos ? view.size() - i : eol - i );
			if ( !comment_line.empty() && comment_line.back() == ' ' )
			{
				set_error( "Trailing spaces are not allowed" );
				return;
			}
			consume_line( view );
			continue;
		}

		break;
	}
}

void Parser::skip_initial( std::string_view &view )
{
	if ( view.starts_with( "%HUML " ) )
	{
		consume_line( view );
	}
	else if ( view.starts_with( "%HUML" ) )
	{
		// "%HUML" not followed by a space is malformed.
		set_error( "Malformed version directive: expected '%HUML <version>'" );
		return;
	}
	skip_to_next_content( view );
}

void Parser::consume_line( std::string_view &view )
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

std::optional<Value> Parser::parse_root( std::string_view &view )
{
	if ( view.empty() )
		return Value{ Null{} };

	return parse_indented_value( view, 0, true );
}

std::optional<Value> Parser::parse_scalar_or_inline( std::string_view &view )
{
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
		return Value{ Null{} };
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

std::optional<Value> Parser::parse_number( std::string_view &view )
{
	// 1. Identify the numeric token boundaries based on HUML delimiters.
	auto end = view.find_first_of( ", \n]}#" );
	std::string_view sub = view.substr( 0, end );

	// 2. Prepare a cleaned string without underscores for parsing.
	std::string s( sub );
	std::erase( s, '_' );

	std::string_view sv = s;
	if ( sv.empty() )
	{
		set_error( "Expected numeric value" );
		return std::nullopt;
	}

	// 3. Handle explicit signs.
	int64_t sign = 1;
	if ( sv.starts_with( '-' ) )
	{
		sign = -1;
		sv.remove_prefix( 1 );
	}
	else if ( sv.starts_with( '+' ) )
	{
		sv.remove_prefix( 1 );
	}

	// 4. Determine base for integer parsing.
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

	// Advance the master view past the token
	advance( view, sub.size() );

	// 5. Attempt Integer Parse
	int64_t i_result = 0;
	auto [i_ptr, i_ec] =
		std::from_chars( sv.data(), sv.data() + sv.size(), i_result, base );

	// Only accept if the entire cleaned string was consumed
	if ( i_ec == std::errc{} && i_ptr == sv.data() + sv.size() )
	{
		return Value{ sign * i_result };
	}

	// 6. Attempt Double Parse (only for base 10)
	if ( base == 10 )
	{
		double d_result = 0.0;
		// Use the full cleaned string 's' to include the sign for from_chars.
		auto [d_ptr, d_ec] = std::from_chars( s.data(), s.data() + s.size(), d_result );

		if ( d_ec == std::errc{} && d_ptr == s.data() + s.size() )
		{
			return Value{ d_result };
		}
	}

	// 7. Fail loudly: if it wasn't a valid number, it must be a quoted string in HUML.
	set_error( "Invalid numeric format; strings must be double-quoted" );
	return std::nullopt;
}

std::optional<List> Parser::parse_multiline_list( std::string_view &view, int indent )
{
	List list;
	while ( !view.empty() )
	{
		if ( !check_indent( view, indent ) )
			break;

		advance( view, static_cast<size_t>( indent ) * 2 );

		if ( !view.starts_with( "- " ) )
		{
			break;
		}

		advance( view, 2 ); // consume "- "

		if ( view.starts_with( ' ' ) )
		{
			set_error( "Only a single space is allowed after the '-' indicator" );
			return std::nullopt;
		}

		auto val = parse_indented_value( view, indent + 1, true );
		if ( !val )
			return std::nullopt;
		list.push_back( std::move( *val ) );
		skip_to_next_content( view );
	}
	return list;
}

std::optional<Dict> Parser::parse_multiline_dict( std::string_view &view, int indent )
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

		if ( view.starts_with( "::" ) )
		{
			advance( view, 2 );
			size_t indicator_line = line_;

			// Must be followed by exactly '\n', optional '# comment', or ' ' + inline content
			bool has_inline_content = false;
			if ( view.starts_with( ' ' ) )
			{
				if ( view.size() >= 2 && view[1] != '\n' && view[1] != '#' )
				{
					if ( view[1] == ' ' )
					{
						set_error( "Only a single space is allowed after '::'" );
						return std::nullopt;
					}
					has_inline_content = true;
					advance( view, 1 );
				}
			}
			else if (
				!view.empty() && !view.starts_with( '\n' ) && !view.starts_with( '#' ) )
			{
				set_error( "Expected newline or ' ' after '::'" );
				return std::nullopt;
			}

			Value val;
			if ( has_inline_content )
			{
				auto v = parse_inline_vector( view );
				if ( !v )
					return std::nullopt;
				val = std::move( *v );
			}
			else
			{
				skip_to_next_content( view );
				auto v = parse_indented_value( view, indent + 1, true );
				if ( !v )
					return std::nullopt;
				if ( !v->is_list() && !v->is_dict() )
				{
					set_error( "Expected vector after '::'", indicator_line );
					return std::nullopt;
				}
				val = std::move( *v );
			}

			if ( dict.find( *key ) != dict.end() )
			{
				set_error( "Duplicate keys are not allowed." );
				return std::nullopt;
			}
			dict[std::move( *key )] = std::move( val );
		}
		else
		{
			advance( view, 1 ); // consume ':'
			size_t indicator_line = line_;

			if ( !view.starts_with( ' ' ) )
			{
				set_error( "Expected exactly one space after ':'" );
				return std::nullopt;
			}
			if ( view.size() >= 2 && view[1] == ' ' )
			{
				set_error( "Only a single space is allowed after ':'" );
				return std::nullopt;
			}
			advance( view, 1 );

			auto v = parse_indented_value( view, indent + 1, true );
			if ( !v )
				return std::nullopt;
			if ( v->is_list() || v->is_dict() )
			{
				set_error(
					"The ':' indicator must be followed by a scalar.", indicator_line );
				return std::nullopt;
			}
			if ( dict.find( *key ) != dict.end() )
			{
				set_error( "Duplicate keys are not allowed." );
				return std::nullopt;
			}
			dict[std::move( *key )] = std::move( *v );
		}

		skip_to_next_content( view );
	}
	return dict;
}

std::optional<Value> Parser::parse_inline_vector( std::string_view &view )
{
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

	if ( is_key( view ) )
	{
		// Inline dict.
		Dict dict;
		while ( !view.empty() )
		{
			auto key = parse_key( view );
			if ( !key )
				break;
			if ( !view.starts_with( ':' ) )
				break;
			advance( view, 1 );

			if ( !view.starts_with( ' ' ) )
			{
				set_error( "Expected exactly one space after ':' in inline dict" );
				return std::nullopt;
			}
			if ( view.size() >= 2 && view[1] == ' ' )
			{
				set_error( "Only a single space is allowed after ':' in inline dict" );
				return std::nullopt;
			}
			advance( view, 1 ); // consume the single space

			auto val = parse_scalar_or_inline( view );
			if ( !val )
				return std::nullopt;
			if ( val->is_list() || val->is_dict() )
			{
				set_error( "Inline dict values must be scalars" );
				return std::nullopt;
			}
			dict[std::move( *key )] = std::move( *val );

			if ( view.starts_with( ',' ) )
			{
				advance( view, 1 );
				if ( !view.starts_with( ' ' ) )
				{
					set_error( "Expected exactly one space after ',' in inline dict" );
					return std::nullopt;
				}
				if ( view.size() >= 2 && view[1] == ' ' )
				{
					set_error(
						"Only a single space is allowed after ',' in inline dict" );
					return std::nullopt;
				}
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
		// Inline list.
		List list;
		while ( !view.empty() )
		{
			auto val = parse_scalar_or_inline( view );
			if ( !val )
				return std::nullopt;
			if ( val->is_list() || val->is_dict() )
			{
				set_error( "Inline list items must be scalars" );
				return std::nullopt;
			}
			list.push_back( std::move( *val ) );

			if ( view.starts_with( ',' ) )
			{
				advance( view, 1 );
				if ( !view.starts_with( ' ' ) )
				{
					set_error( "Expected exactly one space after ',' in inline list" );
					return std::nullopt;
				}
				if ( view.size() >= 2 && view[1] == ' ' )
				{
					set_error(
						"Only a single space is allowed after ',' in inline list" );
					return std::nullopt;
				}
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

std::optional<Value> Parser::parse_indented_value(
	std::string_view &view, int expected_indent, bool allow_multiline_vector )
{
	if ( view.starts_with( '#' ) )
		consume_line( view );

	skip_to_next_content( view );

	// Handle a bare "::" in value position — only valid in a list item context
	// (i.e. "- ::"). The allow_multiline_vector flag gates this.
	if ( allow_multiline_vector && view.starts_with( "::" ) )
	{
		advance( view, 2 );

		bool has_inline_content = false;
		if ( view.starts_with( ' ' ) )
		{
			if ( view.size() >= 2 && view[1] != '\n' && view[1] != '#' )
			{
				if ( view.size() >= 2 && view[1] == ' ' )
				{
					set_error( "Only a single space is allowed after '::'" );
					return std::nullopt;
				}
				has_inline_content = true;
				advance( view, 1 );
			}
		}

		if ( has_inline_content )
			return parse_inline_vector( view );

		// Multi-line: skip to next content then parse the indented vector body.
		skip_to_next_content( view );
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
			return { std::move( *d ) };
		}
	}

	return parse_scalar_or_inline( view );
}

std::optional<MultiLine>
Parser::parse_multiline_string( std::string_view &view, int indent )
{
	advance( view, 3 );
	consume_line( view );

	MultiLine lines;
	// Content lines are indented one extra level relative to the key's indent,
	// so the minimum indentation to strip is (indent) * 2 spaces.
	// (indent here is already expected_indent which equals key_level + 1 for
	//  scalar keys, but the """ sits at key level so indent == key_level + 1
	//  means base_spaces == key_level * 2 + 2).
	size_t base_spaces = static_cast<size_t>( indent ) * 2;

	// The closing """ sits at the key's indentation level, which is (indent - 1)
	// levels, i.e. (indent - 1) * 2 spaces.
	size_t closing_spaces = static_cast<size_t>( indent > 0 ? indent - 1 : 0 ) * 2;

	while ( !view.empty() )
	{
		std::string_view temp = view;
		size_t spaces = 0;
		while ( temp.starts_with( ' ' ) )
		{
			temp.remove_prefix( 1 );
			spaces++;
		}

		if ( temp.starts_with( "\"\"\"" ) && spaces == closing_spaces )
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

std::optional<std::string> Parser::parse_key( std::string_view &view )
{
	if ( view.starts_with( '"' ) )
		return parse_quoted_string( view );

	// Unquoted key: must start with a letter and contain only a-zA-Z0-9_-.
	auto eol = view.find( '\n' );
	auto colon = view.find( ':' );
	if ( colon == std::string_view::npos ||
		 ( eol != std::string_view::npos && colon > eol ) )
		return std::nullopt;

	std::string_view key_view = view.substr( 0, colon );

	// The first character must be a letter.
	if ( key_view.empty() )
		return std::nullopt;
	if ( !( ( key_view[0] >= 'a' && key_view[0] <= 'z' ) ||
			( key_view[0] >= 'A' && key_view[0] <= 'Z' ) ) )
		return std::nullopt;

	// All subsequent characters must be alphanumeric, '_', or '-'.
	for ( size_t i = 1; i < key_view.size(); ++i )
	{
		char c = key_view[i];
		bool valid = ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
			( c >= '0' && c <= '9' ) || c == '_' || c == '-';
		if ( !valid )
		{
			if ( c == ' ' )
				set_error( "Spaces are not allowed before ':' in key" );
			else
				set_error(
					"Invalid character in unquoted key; use a quoted key for special characters" );
			return std::nullopt;
		}
	}

	std::string key( key_view );
	advance( view, colon );
	return key;
}

bool Parser::is_key( std::string_view v )
{
	if ( v.empty() )
		return false;

	// A quoted key is only valid if followed by a colon.
	if ( v[0] == '"' )
	{
		size_t i = 1;
		while ( i < v.size() )
		{
			if ( v[i] == '\\' )
			{
				i += 2;
				continue;
			}
			if ( v[i] == '"' )
			{
				i++;
				return i < v.size() && v[i] == ':';
			}
			i++;
		}
		return false;
	}

	// An unquoted key must start with a letter.
	if ( !( ( v[0] >= 'a' && v[0] <= 'z' ) || ( v[0] >= 'A' && v[0] <= 'Z' ) ) )
		return false;

	// Scan forward to find the colon that terminates the key.
	size_t i = 1;
	while ( i < v.size() )
	{
		char c = v[i];
		if ( c == ':' )
			return true; // reached the key terminator
		bool valid = ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) ||
			( c >= '0' && c <= '9' ) || c == '_' || c == '-';
		if ( !valid )
			return false;
		i++;
	}
	return false; // never found a ':'
}

bool Parser::check_indent( std::string_view view, int level )
{
	if ( view.empty() )
		return false;

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

std::optional<std::string> Parser::parse_quoted_string( std::string_view &view )
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
				default: set_error( "Invalid escape sequence" ); return std::nullopt;
			}
			advance( view, 2 );
		}
		else if ( view[0] == '"' )
		{
			advance( view, 1 );
			return result;
		}
		else if ( view[0] == '\n' )
		{
			// Single-line strings cannot span lines.
			set_error( "Unescaped newline inside quoted string" );
			return std::nullopt;
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

} // namespace huml

