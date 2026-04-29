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

class Theme;

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

	void register_factory( const std::string &intent, FontFactory factory )
	{
		_factories[intent] = std::move( factory );
	}

	FontStack get_stack( const TextStyle &style );
	MathFonts get_math( const TextStyle &style );

private:
	FT_Library _library = nullptr;
	std::map<std::pair<std::string, size_t>, FontFacePtr> _faces;
	std::map<TextStyle, FontStack> _cache;
	std::map<std::string, FontFactory> _factories;
};
