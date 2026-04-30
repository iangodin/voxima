
#include "Theme.h"

#include "MathFonts.h"
#include "freetype_error.hpp"

#include <stdexcept>
#include <filesystem>
#include <format>

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
	auto key = std::make_pair( name, face_index );
	auto it = _faces.find( key );
	if ( it != _faces.end() )
	{
		return { it->second, scale };
	}

	FT_Face face = nullptr;
	std::filesystem::path path = "fonts";
	path /= name;

	if ( const auto err = FT_New_Face( _library, path.string().c_str(), static_cast<FT_Long>( face_index ), &face ); err != 0 )
	{
		throw std::runtime_error( std::format( "Failed to load font {}: {}", path.string(), ft_error_string( err ) ) );
	}

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
