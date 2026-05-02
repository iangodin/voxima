
#pragma once

#include <map>
#include <string_view>
#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <cstdint>

namespace huml
{

struct Value;
using Dict = std::map<std::string, Value>;
using List = std::vector<Value>;
using MultiLine = std::vector<std::string>;

struct ParseError
{
	std::string message;
	size_t line = 0;
};

using ParseResult = std::variant<Value, ParseError>;

struct Missing
{
};

using Null = std::monostate;

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
	const Value &operator[]( const std::string &key ) const;
	const Value &operator[]( size_t i ) const;

	size_t size() const;
	bool empty() const;

private:
};

class Document
{
public:
	Document() = default;

	Document( const Document & ) = default;
	Document( Document && ) = default;

	~Document() = default;

	Document &operator=( const Document & ) = default;
	Document &operator=( Document && ) = default;

	ParseResult parse( std::string source );

	const Value &root() const
	{
		if ( _impl )
		{
			return _impl->root;
		}
		else
		{
			return _nil;
		}
	}

private:
	struct Impl
	{
		std::string source;
		Value root;
	};

	std::shared_ptr<Impl> _impl = nullptr;
	Value _nil = {};
};

} // namespace huml
