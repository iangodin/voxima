#include "Highlight.h"

#include <code/Queries.h>

#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-c.h>
#include <tree_sitter/tree-sitter-cpp.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <regex>
#include <stdexcept>

namespace code
{

namespace
{

// A predicate on a pattern's captures, from the query (#match?, #eq?, #any-of? and their negations). The
// C library only lists them; checking them is up to whoever runs the query.
struct Predicate
{
	std::string name;         // without the '?', e.g. "match", "not-eq"
	uint32_t capture = 0;     // the capture it is about (always the first argument)
	std::vector<std::string> strings;
	std::optional<uint32_t> other_capture; // #eq? @a @b
	std::optional<std::regex> regex;
};

// A grammar with its highlight query, made once and shared (a query is not changed by running it)
class Grammar
{
public:
	Grammar( const TSLanguage *language, const std::string &query_text ) :
		_language( language )
	{
		uint32_t error_offset = 0;
		TSQueryError error = TSQueryErrorNone;
		_query = ts_query_new( language, query_text.data(), static_cast<uint32_t>( query_text.size() ), &error_offset, &error );
		if ( !_query )
		{
			const size_t line = static_cast<size_t>( std::count( query_text.begin(), query_text.begin() + error_offset, '\n' ) ) + 1;
			throw std::runtime_error( "highlight query does not fit its grammar (error " + std::to_string( static_cast<int>( error ) ) + " at line " + std::to_string( line ) + ")" );
		}

		for ( uint32_t pattern = 0; pattern < ts_query_pattern_count( _query ); ++pattern )
		{
			_predicates.push_back( read_predicates( pattern ) );
		}
	}

	~Grammar()
	{
		ts_query_delete( _query );
	}

	Grammar( const Grammar & ) = delete;
	Grammar &operator=( const Grammar & ) = delete;

	const TSLanguage *language() const
	{
		return _language;
	}

	const TSQuery *query() const
	{
		return _query;
	}

	std::string_view capture_name( uint32_t id ) const
	{
		uint32_t length = 0;
		const char *name = ts_query_capture_name_for_id( _query, id, &length );
		return { name, length };
	}

	// Whether a match passes its pattern's predicates
	bool passes( const TSQueryMatch &match, std::string_view source ) const
	{
		auto text_of = [&]( uint32_t capture ) -> std::optional<std::string_view>
		{
			for ( uint16_t i = 0; i < match.capture_count; ++i )
			{
				if ( match.captures[i].index == capture )
				{
					const TSNode node = match.captures[i].node;
					const uint32_t start = ts_node_start_byte( node );
					return source.substr( start, ts_node_end_byte( node ) - start );
				}
			}
			return std::nullopt;
		};

		for ( const Predicate &p: _predicates[match.pattern_index] )
		{
			const auto text = text_of( p.capture );
			if ( !text )
			{
				continue;
			}
			bool ok = true;
			if ( p.name == "match" || p.name == "not-match" )
			{
				ok = p.regex && std::regex_search( text->begin(), text->end(), *p.regex );
			}
			else if ( p.name == "eq" || p.name == "not-eq" )
			{
				if ( p.other_capture )
				{
					ok = text == text_of( *p.other_capture );
				}
				else
				{
					ok = !p.strings.empty() && *text == p.strings.front();
				}
			}
			else if ( p.name == "any-of" || p.name == "not-any-of" )
			{
				ok = std::find( p.strings.begin(), p.strings.end(), *text ) != p.strings.end();
			}
			else
			{
				continue; // not one we know: it does not rule the match out
			}
			if ( p.name.starts_with( "not-" ) ? ok : !ok )
			{
				return false;
			}
		}
		return true;
	}

private:
	// A string of the query (the length is set by the call, so it is read after it)
	std::string string_value( uint32_t id ) const
	{
		uint32_t length = 0;
		const char *text = ts_query_string_value_for_id( _query, id, &length );
		return std::string( text, length );
	}

	std::vector<Predicate> read_predicates( uint32_t pattern ) const
	{
		std::vector<Predicate> out;
		uint32_t count = 0;
		const TSQueryPredicateStep *steps = ts_query_predicates_for_pattern( _query, pattern, &count );

		// Each predicate: its name (a string), then its arguments, then a DONE
		std::vector<const TSQueryPredicateStep *> current;
		for ( uint32_t i = 0; i < count; ++i )
		{
			if ( steps[i].type != TSQueryPredicateStepTypeDone )
			{
				current.push_back( &steps[i] );
				continue;
			}
			if ( current.size() >= 2 && current[0]->type == TSQueryPredicateStepTypeString && current[1]->type == TSQueryPredicateStepTypeCapture )
			{
				Predicate p;
				p.name = string_value( current[0]->value_id );
				if ( p.name.ends_with( '?' ) )
				{
					p.name.pop_back();
				}
				p.capture = current[1]->value_id;
				for ( size_t k = 2; k < current.size(); ++k )
				{
					if ( current[k]->type == TSQueryPredicateStepTypeCapture )
					{
						p.other_capture = current[k]->value_id;
					}
					else
					{
						p.strings.push_back( string_value( current[k]->value_id ) );
					}
				}
				if ( ( p.name == "match" || p.name == "not-match" ) && !p.strings.empty() )
				{
					p.regex.emplace( p.strings.front() );
				}
				out.push_back( std::move( p ) );
			}
			current.clear();
		}
		return out;
	}

	const TSLanguage *_language;
	TSQuery *_query = nullptr;
	std::vector<std::vector<Predicate>> _predicates; // by pattern
};

const Grammar &grammar( Language language )
{
	switch ( language )
	{
		case Language::C:
		{
			static const Grammar c( tree_sitter_c(), std::string( queries::C_HIGHLIGHTS ) );
			return c;
		}
		case Language::CPP:
		{
			static const Grammar cpp( tree_sitter_cpp(), std::string( queries::C_HIGHLIGHTS ) + "\n" + std::string( queries::CPP_HIGHLIGHTS ) );
			return cpp;
		}
	}
	throw std::invalid_argument( "no grammar for that language" );
}

}

std::optional<Language> language_named( std::string_view name )
{
	std::string lower( name );
	std::transform( lower.begin(), lower.end(), lower.begin(), []( unsigned char c ) { return static_cast<char>( std::tolower( c ) ); } );
	if ( lower == "c" )
	{
		return Language::C;
	}
	if ( lower == "cpp" || lower == "c++" || lower == "cxx" )
	{
		return Language::CPP;
	}
	return std::nullopt;
}

std::vector<Span> highlight( Language language, std::string_view source )
{
	const Grammar &g = grammar( language );

	std::unique_ptr<TSParser, decltype( &ts_parser_delete )> parser( ts_parser_new(), ts_parser_delete );
	if ( !ts_parser_set_language( parser.get(), g.language() ) )
	{
		throw std::runtime_error( "tree-sitter cannot read this grammar (its version is too old or too new)" );
	}
	std::unique_ptr<TSTree, decltype( &ts_tree_delete )> tree(
		ts_parser_parse_string( parser.get(), nullptr, source.data(), static_cast<uint32_t>( source.size() ) ), ts_tree_delete );
	if ( !tree )
	{
		return {};
	}

	// Every capture of every match that passes its predicates
	struct Capture
	{
		uint32_t start;
		uint32_t end;
		uint32_t pattern;
		std::string_view name;
	};
	std::vector<Capture> captures;
	std::unique_ptr<TSQueryCursor, decltype( &ts_query_cursor_delete )> cursor( ts_query_cursor_new(), ts_query_cursor_delete );
	ts_query_cursor_exec( cursor.get(), g.query(), ts_tree_root_node( tree.get() ) );
	TSQueryMatch match;
	while ( ts_query_cursor_next_match( cursor.get(), &match ) )
	{
		if ( !g.passes( match, source ) )
		{
			continue;
		}
		for ( uint16_t i = 0; i < match.capture_count; ++i )
		{
			const TSNode node = match.captures[i].node;
			const uint32_t start = ts_node_start_byte( node );
			const uint32_t end = ts_node_end_byte( node );
			if ( end > start )
			{
				captures.push_back( { start, end, match.pattern_index, g.capture_name( match.captures[i].index ) } );
			}
		}
	}

	// Paint them over each other: bigger parts first, so the smaller ones inside them win, and among parts the
	// same size, earlier patterns first, so later ones win
	std::stable_sort(
		captures.begin(),
		captures.end(),
		[]( const Capture &a, const Capture &b )
		{
			const uint32_t size_a = a.end - a.start;
			const uint32_t size_b = b.end - b.start;
			return size_a != size_b ? size_a > size_b : a.pattern < b.pattern;
		} );
	std::vector<const Capture *> owner( source.size(), nullptr );
	for ( const Capture &c: captures )
	{
		std::fill( owner.begin() + c.start, owner.begin() + c.end, &c );
	}

	// Runs of the same capture, as spans
	std::vector<Span> spans;
	for ( size_t i = 0; i < owner.size(); )
	{
		const Capture *c = owner[i];
		size_t j = i + 1;
		while ( j < owner.size() && owner[j] == c )
		{
			++j;
		}
		if ( c )
		{
			spans.push_back( { i, j, std::string( c->name ) } );
		}
		i = j;
	}
	return spans;
}

}
