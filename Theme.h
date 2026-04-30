#pragma once

#include "TextStyle.h"
#include "FontStack.h"
#include "FontFace.h"
#include "MathFonts.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <memory>
#include <functional>
#include <map>
#include <string>
#include <tuple>

class Theme;

struct FontKey
{
	std::string name;
	size_t face_index;
	int weight;
	int width;
	int slant;
	int style1;
	int style2;

	bool operator<( const FontKey &other ) const
	{
		return std::tie( name, face_index, weight, width, slant, style1, style2 ) <
			   std::tie( other.name, other.face_index, other.weight, other.width, other.slant, other.style1, other.style2 );
	}
};

using FontFactory = std::function<FontStack( const TextStyle &, Theme & )>;

class Theme
{
public:
	Theme();
	~Theme();

	Theme( const Theme & ) = delete;
	Theme &operator=( const Theme & ) = delete;

	Theme( Theme && );
	Theme &operator=( Theme && );

	FontFace load_font( const std::string &name, size_t face_index, float scale );
	FontFace load_variable_font( const std::string &name, size_t face_index, float scale, int weight, int width, int slant );
	FontFace load_emoji_font( const std::string &name, size_t face_index, float scale, int outline, int fill );

	void register_factory( const std::string &intent, FontFactory factory )
	{
		_factories[intent] = std::move( factory );
	}

	FontStack get_stack( const TextStyle &style );
	MathFonts get_math( const TextStyle &style );

	constexpr static float FONT_SCALE = 120.F;

private:
	FontFace load_font_internal( const FontKey &key, float scale );

	FT_Library _library = nullptr;
	std::map<FontKey, FontFacePtr> _faces;
	std::map<TextStyle, FontStack> _cache;
	std::map<std::string, FontFactory> _factories;
};
