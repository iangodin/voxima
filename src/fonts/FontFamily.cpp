#include "FontFamily.h"

#include <core/Theme.h>

#include <format>
#include <stdexcept>
#include <vector>

namespace
{

// The weights to try for a weight, from it toward regular
std::vector<Weight> toward_regular( Weight weight )
{
	switch ( weight )
	{
		case Weight::BLACK: return { Weight::BLACK, Weight::BOLD, Weight::REGULAR };
		case Weight::BOLD: return { Weight::BOLD, Weight::REGULAR };
		case Weight::LIGHT: return { Weight::LIGHT, Weight::REGULAR };
		default: return { Weight::REGULAR };
	}
}

const char *style_keys[][2] = {
	{ "light", "light_italic" },
	{ "regular", "italic" },
	{ "bold", "bold_italic" },
	{ "black", "black_italic" },
};

constexpr Weight style_weights[] = { Weight::LIGHT, Weight::REGULAR, Weight::BOLD, Weight::BLACK };

[[noreturn]] void fail( std::string_view family, const std::string &what )
{
	throw std::runtime_error( std::format( "font family '{}': {}", family, what ) );
}

std::string string_of( std::string_view family, std::string_view key, const huml::Value &value )
{
	if ( !value.is_string() )
	{
		fail( family, std::format( "'{}' must be a file name in quotes", key ) );
	}
	return value.as_string();
}

// The files a dict names by its style keys, and the keys that are not styles left to the caller (`other` is called for them)
template<typename Other>
FontFamily::Files read_files( std::string_view family, const huml::Dict &dict, Other &&other )
{
	FontFamily::Files files;
	for ( const auto &[key, value]: dict )
	{
		bool style = false;
		for ( size_t w = 0; w < 4 && !style; ++w )
		{
			for ( int italic = 0; italic < 2; ++italic )
			{
				if ( key == style_keys[w][italic] )
				{
					files.set( style_weights[w], italic == 1, string_of( family, key, value ) );
					style = true;
					break;
				}
			}
		}
		if ( !style )
		{
			other( key, value );
		}
	}
	return files;
}

Weight weight_of( std::string_view family, std::string_view key )
{
	for ( size_t w = 0; w < 4; ++w )
	{
		if ( key == style_keys[w][0] )
		{
			return style_weights[w];
		}
	}
	fail( family, std::format( "'{}' is not a weight (light, regular, bold, black)", key ) );
}

}

const std::string &FontFamily::Files::pick( Weight weight, bool italic ) const
{
	static const std::string none;
	const auto weights = toward_regular( weight );

	// The same slant, the weight moved toward regular; then (for an italic) the upright ones
	for ( int slant = italic ? 1 : 0; slant >= 0; --slant )
	{
		for ( Weight w: weights )
		{
			if ( auto it = _files.find( { w, slant == 1 } ); it != _files.end() )
			{
				return it->second;
			}
		}
	}

	// A family with only, say, a light: that
	for ( Weight w: { Weight::REGULAR, Weight::LIGHT, Weight::BOLD, Weight::BLACK } )
	{
		for ( bool i: { false, true } )
		{
			if ( auto it = _files.find( { w, i } ); it != _files.end() )
			{
				return it->second;
			}
		}
	}
	return none;
}

FontFamily FontFamily::from_huml( std::string_view name, const huml::Value &value )
{
	FontFamily family;
	family._name = std::string( name );
	if ( !value.is_dict() )
	{
		fail( name, "it must be a dict (of files, or a variable font, a collection or a notdef font)" );
	}
	const huml::Dict &dict = value.as_dict();

	auto has = [&]( const char *key ) { return dict.find( key ) != dict.end(); };
	const int forms = static_cast<int>( has( "variable" ) ) + static_cast<int>( has( "collection" ) ) + static_cast<int>( has( "notdef" ) );
	if ( forms > 1 )
	{
		fail( name, "it is more than one of variable, collection and notdef" );
	}

	if ( has( "variable" ) )
	{
		family._kind = Kind::VARIABLE;
		for ( const auto &[key, v]: dict )
		{
			if ( key == "variable" )
				family._file = string_of( name, key, v );
			else if ( key == "face" && v.is_int() )
				family._face = static_cast<size_t>( v.as_int() );
			else if ( key == "weight" && v.is_int() )
				family._weight = static_cast<int>( v.as_int() );
			else if ( key == "width" && v.is_int() )
				family._width = static_cast<int>( v.as_int() );
			else if ( key == "slant" && v.is_int() )
				family._slant = static_cast<int>( v.as_int() );
			else
				fail( name, std::format( "unknown key '{}' for a variable font (variable, face, weight, width, slant)", key ) );
		}
		return family;
	}

	if ( has( "collection" ) )
	{
		family._kind = Kind::COLLECTION;
		for ( const auto &[key, v]: dict )
		{
			if ( key == "collection" )
			{
				family._file = string_of( name, key, v );
			}
			else if ( key == "faces" && v.is_dict() )
			{
				for ( const auto &[weight, index]: v.as_dict() )
				{
					if ( !index.is_int() )
						fail( name, std::format( "the face for '{}' must be a number", weight ) );
					family._faces[weight_of( name, weight )] = static_cast<size_t>( index.as_int() );
				}
			}
			else
			{
				fail( name, std::format( "unknown key '{}' for a collection (collection, faces)", key ) );
			}
		}
		if ( family._faces.empty() )
		{
			fail( name, "a collection needs `faces`: the face for each weight" );
		}
		return family;
	}

	if ( has( "notdef" ) )
	{
		family._kind = Kind::NOTDEF;
		family._file = string_of( name, "notdef", dict.at( "notdef" ) );
		if ( dict.size() > 1 )
		{
			fail( name, "a notdef font has only its file" );
		}
		return family;
	}

	family._kind = Kind::FILES;
	family._files = read_files(
		name,
		dict,
		[&]( const std::string &key, const huml::Value &v )
		{
			if ( key == "face" && v.is_int() )
			{
				family._face = static_cast<size_t>( v.as_int() );
			}
			else if ( key == "small" && v.is_dict() )
			{
				family._small = read_files( name, v.as_dict(), [&]( const std::string &k, const huml::Value & ) { fail( name, std::format( "unknown key '{}' in small", k ) ); } );
				if ( family._small->empty() )
					fail( name, "`small` has no files" );
			}
			else
			{
				fail( name, std::format( "unknown key '{}' (light, regular, bold, black, their _italic, face, small; or variable, collection, notdef)", key ) );
			}
		} );
	if ( family._files.empty() )
	{
		fail( name, "it has no files" );
	}
	return family;
}

FontFace FontFamily::load( Theme &theme, const FontProperties &properties, float size ) const
{
	const Weight weight = properties.weight.value_or( Weight::REGULAR );
	const bool italic = properties.slant == Slant::ITALIC || properties.slant == Slant::OBLIQUE;

	switch ( _kind )
	{
		case Kind::VARIABLE:
		{
			int w = 400;
			if ( weight == Weight::LIGHT )
				w = 300;
			else if ( weight == Weight::BOLD )
				w = 700;
			else if ( weight == Weight::BLACK )
				w = 900;
			return theme.load_variable_font( _file, _face, size, _weight.value_or( w ), _width.value_or( 100 ), _slant.value_or( italic ? 1 : 0 ) );
		}
		case Kind::COLLECTION:
		{
			size_t face = _faces.begin()->second;
			for ( Weight w: toward_regular( weight ) )
			{
				if ( auto it = _faces.find( w ); it != _faces.end() )
				{
					face = it->second;
					break;
				}
			}
			return theme.load_font( _file, face, size );
		}
		case Kind::NOTDEF:
			return theme.load_notdef_font( _file, size );
		case Kind::FILES:
		{
			const Scale scale = properties.scale.value_or( Scale::MEDIUM );
			const bool small = scale == Scale::TINY || scale == Scale::SMALL;
			const Files &files = small && _small ? *_small : _files;
			return theme.load_font( files.pick( weight, italic ), _face, size );
		}
	}
	throw std::logic_error( "font family of no kind" );
}
