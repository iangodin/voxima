#pragma once

#include "fonts/FontFace.h"

#include <optional>
#include <vector>

class FontStack
{
public:
	// The first font in the stack that has the character, or nothing if none does
	std::optional<GlyphEntry> find_codepoint( char32_t codepoint ) const;

	// The same, but throws when no font has it
	GlyphEntry lookup_codepoint( char32_t codepoint ) const;

	void prepend( FontFace face )
	{
		_stack.insert( _stack.begin(), std::move( face ) );
	}

	void append( FontFace face )
	{
		_stack.push_back( std::move( face ) );
	}

	const FontFace &font( size_t i ) const
	{
		return _stack.at( i );
	}

	auto begin( void ) const
	{
		return _stack.begin();
	}

	auto end( void ) const
	{
		return _stack.end();
	}

	size_t size( void ) const
	{
		return _stack.size();
	}

private:
	std::vector<FontFace> _stack;
};
