
#pragma once

#include <map>
#include <string_view>
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <cstdint>
#include <iostream>

namespace huml
{

struct Value;
void print( std::ostream &output, const Value &value, const std::string &indent = {} );

using Dict = std::map<std::string, Value, std::less<>>;
using List = std::vector<Value>;
using MultiLine = std::vector<std::string>;

struct Missing
{
};

struct Null
{
};

struct Value
	: std::variant<
		  Null,
		  Missing,
		  bool,
		  int64_t,
		  double,
		  std::string,
		  MultiLine,
		  List,
		  Dict>
{
	using variant::variant;

	// Type checks
	bool is_null() const
	{
		return std::holds_alternative<Null>( *this );
	}

	bool is_missing() const
	{
		return std::holds_alternative<Missing>( *this );
	}

	bool is_bool() const
	{
		return std::holds_alternative<bool>( *this );
	}

	bool is_int() const
	{
		return std::holds_alternative<int64_t>( *this );
	}

	bool is_double() const
	{
		return std::holds_alternative<double>( *this );
	}

	bool is_string() const
	{
		return std::holds_alternative<std::string>( *this );
	}

	bool is_multiline() const
	{
		return std::holds_alternative<MultiLine>( *this );
	}

	bool is_dict() const
	{
		return std::holds_alternative<Dict>( *this );
	}

	bool is_list() const
	{
		return std::holds_alternative<List>( *this );
	}

	// Convenient getters
	bool as_bool() const
	{
		return std::get<bool>( *this );
	}

	int64_t as_int() const
	{
		return std::get<int64_t>( *this );
	}

	double as_double() const
	{
		return std::get<double>( *this );
	}

	const std::string &as_string() const
	{
		return std::get<std::string>( *this );
	}

	std::string_view as_string( std::string_view def ) const
	{
		if ( auto *str = std::get_if<std::string>( this ) )
		{
			return *str;
		}
		else
		{
			return def;
		}
	}

	const MultiLine &as_multiline() const
	{
		return std::get<MultiLine>( *this );
	}

	const Dict &as_dict() const
	{
		return std::get<Dict>( *this );
	}

	const List &as_list() const
	{
		return std::get<List>( *this );
	}

	// Dictionary access sugar
	const Value &operator[]( std::string_view key ) const;
	const Value &operator[]( size_t i ) const;

	template<typename ...Args>
	const Value &fetch( Args &&...multikey ) const
	{
		std::reference_wrapper current( *this );
		auto fetch_next = [&]( auto key )
		{
			current = current.get().operator[]( key );
		};

		( fetch_next( std::forward<Args>( multikey ) ), ... );

		return current.get();
	}

	size_t size() const;
	bool empty() const;

	friend std::ostream &operator<<( std::ostream &os, const Value &value )
	{
		print( os, value );
		return os;
	}
};

Value merge( const Value &master, const Value &variant );

struct ParseError
{
	std::string message;
	size_t line = 0;

	friend std::ostream &operator<<( std::ostream &os, const ParseError &e )
	{
		os << "line " << e.line << ": " << e.message;
		return os;
	}
};

using ParseResult = std::variant<huml::Value, ParseError>;

inline std::ostream &operator<<( std::ostream &os, const ParseResult &result )
{
	std::visit( [&]( const auto &v )
	{
		os << v << '\n';
	},
	result );
	return os;
}

class Parser
{
public:
	explicit Parser( std::string_view source ) :
		view_( source )
	{
	}

	ParseResult parse();

private:
	std::string_view view_;
	size_t line_ = 1;
	std::optional<ParseError> last_error_;

	void advance( std::string_view &view, size_t n );

	bool check_end_of_line( std::string_view view );

	void set_error( std::string message )
	{
		if ( !last_error_ )
		{
			last_error_ = ParseError{ std::move( message ), line_ };
		}
	}

	void set_error( std::string message, size_t line )
	{
		if ( !last_error_ )
		{
			last_error_ = ParseError{ std::move( message ), line };
		}
	}

	void skip_to_next_content( std::string_view &view );

	void skip_initial( std::string_view &view );

	void consume_line( std::string_view &view );

	std::optional<Value> parse_root( std::string_view &view );

	std::optional<Value> parse_scalar_or_inline( std::string_view &view );

	std::optional<Value> parse_number( std::string_view &view );

	std::optional<List> parse_multiline_list( std::string_view &view, int indent );

	std::optional<Dict> parse_multiline_dict( std::string_view &view, int indent );

	std::optional<Value> parse_inline_vector( std::string_view &view );

	std::optional<Value> parse_indented_value(
		std::string_view &view, int expected_indent, bool allow_multiline_vector );

	std::optional<MultiLine> parse_multiline_string( std::string_view &view, int indent );

	std::optional<std::string> parse_key( std::string_view &view );

	bool is_key( std::string_view v );

	bool check_indent( std::string_view view, int level );

	std::optional<std::string> parse_quoted_string( std::string_view &view );
};

}
