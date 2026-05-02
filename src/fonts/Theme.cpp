
#include "fonts/Theme.h"

#include "fonts/MathFonts.h"
#include "utils/FreetypeError.hpp"

#include FT_MULTIPLE_MASTERS_H

#include <stdexcept>
#include <filesystem>
#include <format>
#include <algorithm>

namespace
{

void apply_variations( FT_Face face, const FontKey &key )
{
	if ( !FT_HAS_MULTIPLE_MASTERS( face ) )
	{
		return;
	}

	FT_MM_Var *mm_var = nullptr;
	if ( FT_Get_MM_Var( face, &mm_var ) != 0 )
	{
		return;
	}

	std::vector<FT_Fixed> coords( mm_var->num_axis );
	for ( FT_UInt i = 0; i < mm_var->num_axis; ++i )
	{
		FT_Tag tag = static_cast<FT_Tag>( mm_var->axis[i].tag );
		FT_Fixed value = mm_var->axis[i].def;

		if ( tag == FT_MAKE_TAG( 'w', 'g', 'h', 't' ) )
		{
			value = static_cast<FT_Fixed>( key.weight ) << 16;
		}
		else if ( tag == FT_MAKE_TAG( 'w', 'd', 't', 'h' ) )
		{
			value = static_cast<FT_Fixed>( key.width ) << 16;
		}
		else if ( tag == FT_MAKE_TAG( 's', 'l', 'n', 't' ) )
		{
			value = static_cast<FT_Fixed>( key.slant ) << 16;
		}
		else if ( tag == FT_MAKE_TAG( 'i', 't', 'a', 'l' ) )
		{
			value = ( key.slant > 0 ? 1 : 0 ) << 16;
		}
		else if ( tag == FT_MAKE_TAG( 'O', 'U', 'T', 'F' ) )
		{
			value = static_cast<FT_Fixed>( key.style1 ) << 16;
		}
		else if ( tag == FT_MAKE_TAG( 'F', 'I', 'L', 'L' ) )
		{
			value = static_cast<FT_Fixed>( key.style2 ) << 16;
		}

		coords[i] = std::clamp( value, mm_var->axis[i].minimum, mm_var->axis[i].maximum );
	}

	FT_Set_Var_Design_Coordinates( face, mm_var->num_axis, coords.data() );
	FT_Done_MM_Var( face->glyph->library, mm_var );
}

}

Theme::Theme()
{
	if ( FT_Init_FreeType( &_library ) != 0 )
	{
		throw std::runtime_error( "Failed to initialize FreeType" );
	}
}

Theme::~Theme()
{
	_cache.clear();
	_faces.clear();
	if ( _library )
	{
		FT_Done_FreeType( _library );
	}
}

Theme::Theme( Theme &&other ) :
	_library( other._library ),
	_faces( std::move( other._faces ) ),
	_cache( std::move( other._cache ) ),
	_factories( std::move( other._factories ) )
{
	other._library = nullptr;
}

Theme &Theme::operator=( Theme &&other )
{
	if ( this != &other )
	{
		if ( _library )
		{
			FT_Done_FreeType( _library );
		}
		_library = other._library;
		_faces = std::move( other._faces );
		_cache = std::move( other._cache );
		_factories = std::move( other._factories );
		other._library = nullptr;
	}
	return *this;
}

FontFace Theme::load_font( const std::string &name, size_t face_index, float scale )
{
	return load_variable_font( name, face_index, scale, 400, 100, 0 );
}

FontFace Theme::load_variable_font( const std::string &name, size_t face_index, float scale, int weight, int width, int slant )
{
	return load_font_internal( { name, face_index, weight, width, slant, 0, 0 }, scale );
}

FontFace Theme::load_emoji_font( const std::string &name, size_t face_index, float scale, int outline, int fill )
{
	return load_font_internal( { name, face_index, 400, 100, 0, outline, fill }, scale );
}

FontFace Theme::load_font_internal( const FontKey &key, float scale )
{
	auto it = _faces.find( key );
	if ( it != _faces.end() )
	{
		return { it->second, scale };
	}

	FT_Face face = nullptr;
	std::filesystem::path path = _font_path;
	path /= key.name;

	if ( const auto err = FT_New_Face( _library, path.string().c_str(), static_cast<FT_Long>( key.face_index ), &face ); err != 0 )
	{
		throw std::runtime_error( std::format( "Failed to load font {}: {}", path.string(), ft_error_string( err ) ) );
	}

	apply_variations( face, key );

	const auto ft_size = static_cast<FT_F26Dot6>( scale * FONT_SCALE * 64.F );
	if ( const auto err = FT_Set_Char_Size( face, ft_size, ft_size, 0, 0 ); err != 0)
	{
		throw std::invalid_argument( std::format( "Failed to set char size {}: {}", path.string(), ft_error_string( err ) ) );
	}

	FontFacePtr ptr( face, FT_Done_Face );
	_faces[key] = ptr;
	return { ptr, scale };
}

FontStack Theme::get_stack( const TextStyle &style )
{
	auto it = _cache.find( style );
	if ( it != _cache.end() )
	{
		return it->second;
	}

	auto fit = _factories.find( style.intent() );
	if ( fit == _factories.end() )
	{
		throw std::runtime_error( std::format( "No factory registered for intent: {}", style.intent() ) );
	}

	FontStack stack = fit->second( style, *this );
	_cache[style] = stack;
	return stack;
}

MathFonts Theme::get_math( const TextStyle &style )
{
	if ( style.intent() != "math" )
	{
		throw std::runtime_error( "expected math intent" );
	}

	auto base_style = style.properties();

	auto roman_stack = get_stack( TextStyle( "math", base_style ) );
	auto italic_stack = get_stack( TextStyle( "math", base_style ).italic() );
	auto bold_stack = get_stack( TextStyle( "math", base_style ).bold() );
	auto sans_stack = get_stack( TextStyle( "math", base_style ).sans() );
	auto mono_stack = get_stack( TextStyle( "math", base_style ).mono() );

	return {
		roman_stack.font( 0 ),
		italic_stack.font( 0 ),
		bold_stack.font( 0 ),
		sans_stack.font( 0 ),
		mono_stack.font( 0 )
	};
}
