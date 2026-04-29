#pragma once

#include "Group.h"
#include "Freetype.h"

#include <mfl/font_family.hpp>

#include <string>
#include <memory>

class MFLFontFace;
class Glyph;

class Font
{
public:
	Font( const std::string &path );
	~Font();
	std::unique_ptr<Group> create_text( const std::string &text, float font_size ) const;
    std::unique_ptr<Group> create_math( const std::string &formula, float font_size ) const;

private:
    std::string _path;
	Freetype _ft;
    std::unique_ptr<MFLFontFace> _face;
    std::unique_ptr<Glyph> create_glyph( unsigned int glyph_id, mfl::font_family family ) const;
    void center_group( Group &group ) const;
};

