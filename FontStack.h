#pragma once

#include "FontFace.h"

#include <vector>

class FontStack
{
public:
	GlyphEntry lookup_codepoint( char32_t codepoint ) const;

	void push( FontFace entry )
	{
		_stack.push_back( std::move( entry ) );
	}

	void push( FontFacePtr face, float scale )
	{
		_stack.push_back( { std::move( face ), scale } );
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
