
#include <themes/NewComputerModern.h>

#include <core/Theme.h>

#include <fonts/FontStack.h>
#include <fonts/TextStyle.h>

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
	bool is_bold = props.weight == Weight::BOLD || props.weight == Weight::BLACK;
	bool is_italic = props.slant == Slant::ITALIC || props.slant == Slant::OBLIQUE;
	bool is_light = props.weight == Weight::LIGHT;

	Scale scale = props.scale.value_or( Scale::MEDIUM );
	bool is_small = scale == Scale::TINY || scale == Scale::SMALL;

	std::string family{ base_name };
	if ( base_name == "NewCM" || base_name == "NewCMSans" || base_name == "NewCMUncial" ) {
		family += ( is_small ? "08" : "10" );
	} else if ( base_name == "NewCMMono" ) {
		family += "10";
	}

	if ( family.starts_with( "NewCM08" ) || family.starts_with( "NewCM10" ) ) {
		if ( is_bold && is_italic ) return family + "-BoldItalic.otf";
		if ( is_bold ) return family + "-Bold.otf";
		if ( is_italic ) return family + "-Italic.otf";
		if ( is_light ) return family + "-Book.otf";
		return family + "-Regular.otf";
	}
	if ( family.starts_with( "NewCMMono10" ) ) {
		if ( is_bold && is_italic ) return "NewCMMono10-BoldOblique.otf";
		if ( is_bold ) return "NewCMMono10-Bold.otf";
		if ( is_italic ) return "NewCMMono10-Italic.otf";
		if ( is_light ) return "NewCMMono10-Book.otf";
		return "NewCMMono10-Regular.otf";
	}
	if ( family.starts_with( "NewCMSans" ) ) {
		if ( is_bold && is_italic ) return family + "-BoldOblique.otf";
		if ( is_bold ) return family + "-Bold.otf";
		if ( is_italic ) return family + "-Oblique.otf";
		if ( is_light ) return family + "-Book.otf";
		return family + "-Regular.otf";
	}
	if ( family.starts_with( "NewCMUncial" ) ) {
		if ( is_bold ) return family + "-Bold.otf";
		if ( is_light ) return family + "-Book.otf";
		return family + "-Regular.otf";
	}
	if ( family == "NewCMMath" ) {
		if ( is_bold ) return "NewCMMath-Bold.otf";
		if ( is_light ) return "NewCMMath-Book.otf";
		return "NewCMMath-Regular.otf";
	}
	return family + "-Regular.otf";
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

std::string get_devanagari_filename( const TextProperties &props )
{
	bool is_bold = props.weight == Weight::BOLD || props.weight == Weight::BLACK;
	bool is_light = props.weight == Weight::LIGHT;

	Scale scale = props.scale.value_or( Scale::MEDIUM );
	bool is_small = scale == Scale::TINY || scale == Scale::SMALL;

	std::string family = is_small ? "NewCM08Devanagari" : "NewCM10Devanagari";

	if ( is_bold ) return family + "-Bold.otf";
	if ( is_light ) return family + "-Book.otf";
	return family + "-Regular.otf";
}
}

FontStack intent_prose( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCM" ), 0, font_size ) );
	fonts.push( theme.load_font( get_devanagari_filename( properties ), 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_code( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCMMono" ), 0, font_size ) );
	fonts.push( theme.load_font( get_devanagari_filename( properties ), 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_title( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = 1.2F * theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCMUncial" ), 0, font_size ) );
	fonts.push( theme.load_font( get_devanagari_filename( properties ), 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_annotation( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = 0.8F * theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCM" ), 0, font_size ) );
	fonts.push( theme.load_font( get_devanagari_filename( properties ), 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_label( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = 0.8F * theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCMSans" ), 0, font_size ) );
	fonts.push( theme.load_font( get_devanagari_filename( properties ), 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_math( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCMMath" ), 0, font_size ) );
	fonts.push( theme.load_font( get_devanagari_filename( properties ), 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_caption( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	const float font_size = 0.8F * theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_font_filename( properties, "NewCM" ), 0, font_size ) );
	fonts.push( theme.load_font( get_devanagari_filename( properties ), 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
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
	theme.register_factory( "caption", intent_caption );
	theme.register_factory( "code", intent_code );
	theme.register_factory( "math", intent_math );
	return theme;
}

}
