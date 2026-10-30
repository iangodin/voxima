#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"

#include <algorithm>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

// Tags name visuals so they can be found again, to animate them for instance. Anything can be
// tagged (tag_visual), and the composites tag their own parts (see below).
//
// Lookups take one name or a path of names, from any root: a built formula, a group, the stage's
// visuals. Each name after the first is searched for inside the previous match, so
//
//     find_tag( built, "fraction1", "bar" )
//
// is the bar inside the part tagged "fraction1". Every step is searched level by level, so the
// nearest match wins (a fraction's own bar, not one nested in its numerator), and siblings
// come left to right. A step includes the visual it starts from, so a visual carrying two names
// answers a path of both.
//
// Names the composites give their parts:
//   Fraction     numerator, denominator, bar (not for a stack, which has no rule)
//   Radical      radicand, overline, sign, index (when there is one)
//   Scripts      base, superscript, subscript
//   Limits       base, upper, lower
//   large operator (either form)   base, upper, lower
//   Delimited    open, close, content
//   Accent       base, accent
//   Overline, Underline   content, line
//   Overbrace, Underbrace   content, brace
//   Matrix       cell, and r<row>c<column> for each cell, counted from 0 ("r0c1")

// Add a name to a visual (once)
inline void tag_visual( Visual &visual, std::string name )
{
	if ( std::find( visual.tags.begin(), visual.tags.end(), name ) == visual.tags.end() )
	{
		visual.tags.push_back( std::move( name ) );
	}
}

namespace tags_detail
{

inline bool has_tag( const Visual &visual, std::string_view name )
{
	return std::find( visual.tags.begin(), visual.tags.end(), name ) != visual.tags.end();
}

// Everything under `scope` (and `scope` itself) with the tag, nearest first
inline std::vector<Visual *> matches_in( Visual &scope, std::string_view name )
{
	std::vector<Visual *> found;
	std::vector<Visual *> queue{ &scope };
	for ( size_t i = 0; i < queue.size(); ++i )
	{
		Visual *visual = queue[i];
		if ( has_tag( *visual, name ) )
		{
			found.push_back( visual );
		}
		if ( auto *group = dynamic_cast<Group *>( visual ) )
		{
			for ( auto &child: *group )
			{
				queue.push_back( child.get() );
			}
		}
	}
	return found;
}

}

inline std::vector<Visual *> find_tagged_path( Visual &root, std::initializer_list<std::string_view> path )
{
	std::vector<Visual *> scopes{ &root };
	for ( std::string_view name: path )
	{
		std::vector<Visual *> next;
		for ( Visual *scope: scopes )
		{
			for ( Visual *match: tags_detail::matches_in( *scope, name ) )
			{
				if ( std::find( next.begin(), next.end(), match ) == next.end() )
				{
					next.push_back( match );
				}
			}
		}
		scopes = std::move( next );
		if ( scopes.empty() )
		{
			break;
		}
	}
	return path.size() == 0 ? std::vector<Visual *>{} : scopes;
}

// Every visual at the end of the path, nearest first
template<typename... Names>
std::vector<Visual *> find_tagged( Visual &root, Names... names )
{
	static_assert( sizeof...( names ) >= 1, "give at least one tag name" );
	return find_tagged_path( root, { std::string_view( names )... } );
}

// The first of those, or nullptr
template<typename... Names>
Visual *find_tag( Visual &root, Names... names )
{
	auto found = find_tagged( root, names... );
	return found.empty() ? nullptr : found.front();
}

// The same as the given type; nullptr if there is none or it isn't one
template<typename T, typename... Names>
T *find_tag_as( Visual &root, Names... names )
{
	return dynamic_cast<T *>( find_tag( root, names... ) );
}

// The tag make_styled_text gives the group of glyphs of word `index`: "word0", "word1", ... counting
// through the whole text, using the same split_words() as the narration timing
inline std::string word_tag( size_t index )
{
	return "word" + std::to_string( index );
}

// Whether a tag is one of those ("word" and then digits only)
inline bool is_word_tag( std::string_view tag )
{
	return tag.size() > 4 && tag.substr( 0, 4 ) == "word" &&
		   std::all_of( tag.begin() + 4, tag.end(), []( char c ) { return c >= '0' && c <= '9'; } );
}

// The group of glyphs for word `index` of laid-out text, or nullptr if there is no such word.
inline Visual *find_word( Visual &root, size_t index )
{
	return find_tag( root, word_tag( index ) );
}
