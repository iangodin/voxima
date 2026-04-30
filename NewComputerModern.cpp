#include "NewComputerModern.h"

#include "Theme.h"
#include "FontStack.h"
#include "TextStyle.h"

namespace NewComputerModern
{

namespace
{
float theme_font_size( Scale scale )
{
	float font_size = 0.3F;
	switch ( scale )
	{
		case Scale::TINY: font_size = 0.1F; break;
		case Scale::SMALL: font_size = 0.25F; break;
		case Scale::MEDIUM: font_size = 0.3F; break;
		case Scale::LARGE: font_size = 0.75F; break;
		case Scale::HUGE: font_size = 1.5F; break;
	}
	return font_size;
}

std::string get_font_filename( const TextProperties &props, std::string_view base_name )
{
	std::string filename{ base_name };
	// Map properties to filenames based on pattern matching (e.g., "NewCM10-Regular.otf")
	// Simplistic mapping for now based on available files.

	bool is_bold = props.weight == Weight::BOLD;
	bool is_italic = props.slant == Slant::ITALIC || props.slant == Slant::OBLIQUE;
	bool is_light = props.weight == Weight::LIGHT;

	if ( base_name == "NewCM10" ) {
		if ( is_bold && is_italic ) return "NewCM10-BoldItalic.otf";
		if ( is_bold ) return "NewCM10-Bold.otf";
		if ( is_italic ) return "NewCM10-Italic.otf";
		if ( is_light ) return "NewCM10-Book.otf";
		return "NewCM10-Regular.otf";
	}
	if ( base_name == "NewCMMono10" ) {
		if ( is_bold ) return "NewCMMono10-Bold.otf";
		if ( is_italic ) return "NewCMMono10-Italic.otf";
		if ( is_light ) return "NewCMMono10-Book.otf";
		return "NewCMMono10-Regular.otf";
	}
	if ( base_name == "NewCMSans10" ) {
		if ( is_bold && is_italic ) return "NewCMSans10-BoldOblique.otf";
		if ( is_bold ) return "NewCMSans10-Bold.otf";
		if ( is_italic ) return "NewCMSans10-Oblique.otf";
		if ( is_light ) return "NewCMSans10-Book.otf";
		return "NewCMSans10-Regular.otf";
	}
	if ( base_name == "NewCMMath" ) {
		if ( is_bold ) return "NewCMMath-Bold.otf";
		if ( is_light ) return "NewCMMath-Book.otf";
		return "NewCMMath-Regular.otf";
	}
	return std::string{ base_name } + "-Regular.otf";
}

std::size_t get_cjk_face_index( const TextProperties &props )
{
	switch ( props.weight.value_or( Weight::REGULAR ) )
	{
		case Weight::LIGHT: return 5;
		case Weight::REGULAR: return 10;
		case Weight::BOLD: return 25;
		case Weight::BLACK: return 30;
		default: return 10;
	}
}
}

FontStack intent_prose( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCM10" ), 0, font_size ) );
	fonts.push( theme.load_font( "NewCM10Devanagari-Regular.otf", 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_code( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCMMono10" ), 0, font_size ) );
	fonts.push( theme.load_font( "NewCM10Devanagari-Regular.otf", 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_title( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = 1.2F * theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCMUncial10" ), 0, font_size ) );
	fonts.push( theme.load_font( "NewCM10Devanagari-Regular.otf", 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_annotation( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = 0.8F * theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCM10" ), 0, font_size ) );
	fonts.push( theme.load_font( "NewCM10Devanagari-Regular.otf", 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_label( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = 0.8F * theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCMSans10" ), 0, font_size ) );
	fonts.push( theme.load_font( "NewCM10Devanagari-Regular.otf", 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_math( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCMMath" ), 0, font_size ) );
	fonts.push( theme.load_font( "NewCM10Devanagari-Regular.otf", 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

Theme create_theme( void )
{
	Theme theme;
	theme.register_factory( "prose", intent_prose );
	theme.register_factory( "title", intent_title );
	theme.register_factory( "label", intent_label );
	theme.register_factory( "annotation", intent_annotation );
	theme.register_factory( "code", intent_code );
	theme.register_factory( "math", intent_math );
	return theme;
}

}
