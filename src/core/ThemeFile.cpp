#include "ThemeFile.h"

#include <core/BuiltinThemes.h>

#include <fonts/FontFamily.h>
#include <themes/ThemeUtils.h>
#include <utils/Huml.h>

#include <format>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>

namespace themes
{

namespace
{

[[noreturn]] void fail( const std::string &where, const std::string &what )
{
	throw std::runtime_error( where.empty() ? what : std::format( "{}: {}", where, what ) );
}

huml::Value parse( std::string_view source, const std::string &where )
{
	const std::string text( source ); // (the parser reads it in place: it must outlive it)
	huml::Parser parser( text );
	huml::ParseResult result = parser.parse();
	if ( !std::holds_alternative<huml::Value>( result ) )
	{
		const auto &error = std::get<huml::ParseError>( result );
		fail( where, std::format( "{} at line {}", error.message, error.line ) );
	}
	return std::get<huml::Value>( result );
}

std::string name_of( const huml::Value &root )
{
	const huml::Value &name = root["meta"]["name"];
	return name.is_string() ? name.as_string() : std::string();
}

// The text of each built-in theme, by name
const std::map<std::string, std::string_view> &builtins()
{
	static const std::map<std::string, std::string_view> all = []
	{
		std::map<std::string, std::string_view> found;
		for ( std::string_view source: BUILTIN_SOURCES )
		{
			const std::string name = name_of( parse( source, "a built-in theme" ) );
			if ( name.empty() )
			{
				fail( "a built-in theme", "it has no meta name" );
			}
			found.emplace( name, source );
		}
		return found;
	}();
	return all;
}

// The text of the theme called `name`: built in, or a file in a directory (or a path)
std::pair<std::string, std::string> find_source( std::string_view name, const std::vector<std::filesystem::path> &directories )
{
	if ( auto it = builtins().find( std::string( name ) ); it != builtins().end() )
	{
		return { std::string( it->second ), std::string( "the built-in theme '" ) + std::string( name ) + "'" };
	}

	std::vector<std::filesystem::path> candidates;
	const std::filesystem::path given{ std::string( name ) };
	if ( given.has_parent_path() || given.extension() == ".huml" )
	{
		candidates.push_back( given );
	}
	for ( const auto &directory: directories )
	{
		candidates.push_back( directory / ( std::string( name ) + ".huml" ) );
	}
	for ( const auto &path: candidates )
	{
		std::ifstream in( path );
		if ( in.is_open() )
		{
			std::ostringstream text;
			text << in.rdbuf();
			return { text.str(), path.string() };
		}
	}

	std::string known;
	for ( const auto &[builtin, source]: builtins() )
	{
		known += ( known.empty() ? "" : ", " ) + builtin;
	}
	fail( "", std::format( "no theme '{}': the built-in ones are {}, and there is no {}.huml in the theme directories", name, known, name ) );
}

// The file with what it inherits under it, all merged
huml::Value resolve( const huml::Value &file, const std::string &where, const std::vector<std::filesystem::path> &directories, std::set<std::string> &seen )
{
	const huml::Value &inherit = file["meta"]["inherit"];
	if ( !inherit.is_string() )
	{
		return file;
	}
	if ( !seen.insert( inherit.as_string() ).second )
	{
		fail( where, std::format( "'{}' inherits itself", inherit.as_string() ) );
	}
	const auto [source, origin] = find_source( inherit.as_string(), directories );
	const huml::Value parent = resolve( parse( source, origin ), origin, directories, seen );
	return huml::merge( parent, file );
}

struct Spec
{
	std::string name;
	std::string palette;
	std::map<std::string, FontFamily> families;
	std::map<std::string, std::vector<std::string>> stacks;
	std::map<std::string, std::string> intents;
};

// The families of a stack, with the "...other" ones spliced in
void flatten( const Spec &spec, const std::string &stack, std::vector<std::string> &out, std::set<std::string> &visiting, const std::string &where )
{
	if ( !visiting.insert( stack ).second )
	{
		fail( where, std::format( "stack '{}' contains itself", stack ) );
	}
	for ( const std::string &item: spec.stacks.at( stack ) )
	{
		if ( item.starts_with( "..." ) )
		{
			const std::string other = item.substr( 3 );
			if ( !spec.stacks.contains( other ) )
			{
				fail( where, std::format( "stack '{}' splices '...{}', which is not a stack", stack, other ) );
			}
			flatten( spec, other, out, visiting, where );
		}
		else
		{
			if ( !spec.families.contains( item ) )
			{
				fail( where, std::format( "stack '{}' has '{}', which is not a font family (the families are in `fonts`)", stack, item ) );
			}
			out.push_back( item );
		}
	}
	visiting.erase( stack );
}

Theme build( const huml::Value &root, const std::string &where )
{
	if ( !root.is_dict() )
	{
		fail( where, "a theme file is a dict" );
	}
	for ( const auto &[key, value]: root.as_dict() )
	{
		if ( key != "meta" && key != "palette" && key != "fonts" && key != "stacks" && key != "intents" )
		{
			fail( where, std::format( "unknown section '{}' (meta, palette, fonts, stacks, intents)", key ) );
		}
	}

	auto spec = std::make_shared<Spec>();
	spec->name = name_of( root );
	if ( spec->name.empty() )
	{
		fail( where, "it needs `meta:: name: \"...\"`" );
	}

	const huml::Value &palette = root["palette"];
	if ( !palette.is_missing() )
	{
		if ( !palette.is_string() )
		{
			fail( where, "`palette` is the name of a palette, in quotes" );
		}
		spec->palette = palette.as_string();
		Palette::get( spec->palette ); // a name that is not there throws, saying which are
	}

	const huml::Value &fonts = root["fonts"];
	if ( fonts.is_dict() )
	{
		for ( const auto &[name, value]: fonts.as_dict() )
		{
			spec->families.emplace( name, FontFamily::from_huml( name, value ) );
		}
	}

	const huml::Value &stacks = root["stacks"];
	if ( stacks.is_dict() )
	{
		for ( const auto &[name, value]: stacks.as_dict() )
		{
			if ( !value.is_list() )
			{
				fail( where, std::format( "stack '{}' is a list of family names in quotes: `{}:: \"a\", \"b\"`", name, name ) );
			}
			std::vector<std::string> items;
			for ( const huml::Value &item: value.as_list() )
			{
				if ( !item.is_string() )
				{
					fail( where, std::format( "stack '{}' has something that is not a family name in quotes", name ) );
				}
				items.push_back( item.as_string() );
			}
			spec->stacks.emplace( name, std::move( items ) );
		}
	}

	// Every stack must hold, all through, families that are there
	std::map<std::string, std::vector<std::string>> flat;
	for ( const auto &[name, items]: spec->stacks )
	{
		std::set<std::string> visiting;
		flatten( *spec, name, flat[name], visiting, where );
	}

	const huml::Value &intents = root["intents"];
	if ( intents.is_dict() )
	{
		for ( const auto &[intent, value]: intents.as_dict() )
		{
			if ( !value.is_string() || !spec->stacks.contains( value.as_string() ) )
			{
				fail( where, std::format( "intent '{}' uses a stack that is not there (the stacks are in `stacks`)", intent ) );
			}
			spec->intents.emplace( intent, value.as_string() );
		}
	}

	Theme theme( spec->name );
	if ( !spec->palette.empty() )
	{
		// A theme that names a palette uses it whole, background included; black is only what a theme with
		// no palette at all starts with (Theme's own default, Palette::house( "one-dark" )).
		theme.set_palette( Palette::get( spec->palette ) );
	}
	for ( const auto &[intent, stack]: spec->intents )
	{
		std::vector<std::string> families = flat.at( stack );
		theme.register_factory(
			intent,
			[spec, families]( const FontStyle &style, Theme &theme )
			{
				const FontProperties properties = style.properties();
				const float size = ThemeUtils::get_theme_font_size( properties );
				FontStack stack;
				for ( const std::string &family: families )
				{
					stack.append( spec->families.at( family ).load( theme, properties, size ) );
				}
				return stack;
			} );
	}
	return theme;
}

}

std::vector<std::string> builtin_names()
{
	std::vector<std::string> names;
	for ( const auto &[name, source]: builtins() )
	{
		names.push_back( name );
	}
	return names;
}

Theme load( std::string_view name, const std::vector<std::filesystem::path> &directories )
{
	const auto [source, origin] = find_source( name, directories );
	std::set<std::string> seen{ std::string( name ) };
	return build( resolve( parse( source, origin ), origin, directories, seen ), origin );
}

Theme from_text( std::string_view source, const std::vector<std::filesystem::path> &directories )
{
	std::set<std::string> seen;
	return build( resolve( parse( source, "the theme" ), "the theme", directories, seen ), "the theme" );
}

}
