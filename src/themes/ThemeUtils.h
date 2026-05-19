#pragma once

#include <fonts/FontStyle.h>

#include <core/Theme.h>

#include <string>
#include <string_view>

namespace ThemeUtils
{

float get_theme_font_size( Scale scale );

struct FontParams
{
	int weight;
	int width;
	int slant;
};

FontParams get_font_params( const FontProperties &props );

std::string get_amiri_filename( const FontProperties &props );
std::string get_fira_filename( const FontProperties &props, std::string_view base_name );
std::string get_ibmplexsansarabic_filename( const FontProperties &props );
std::string get_ibmplexmono_filename( const FontProperties &props );
std::string get_ibmplexsansjp_filename( const FontProperties &props );
std::string get_tajawal_filename( const FontProperties &props );
std::string get_newcm_filename( const FontProperties &props, std::string_view base_name );
std::string get_devanagari_filename( const FontProperties &props );

std::size_t get_cjk_face_index( const FontProperties &props );

}
