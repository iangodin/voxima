#pragma once

#include "Group.h"
#include "Freetype.h"

#include <mfl/font_family.hpp>

#include <string>
#include <memory>

class MFLFontFace;
class Path;

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
    std::unique_ptr<Path> get_glyph_path(unsigned int glyph_id, float scale, mfl::font_family family ) const;
    void center_group( Group &group) const;
};
