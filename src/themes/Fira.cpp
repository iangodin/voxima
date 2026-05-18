#include <core/Theme.h>

#include <themes/Fira.h>

#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

namespace Fira
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

std::string get_fira_filename( const FontProperties &props, std::string_view base_name )
{
	std::string family{ base_name };

	// Handle Width
	if ( base_name == "FiraSans" ) {
		Width width = props.width.value_or( Width::STANDARD );
		if ( width == Width::COMPRESSED ) family = "FiraSansCompressed";
		else if ( width == Width::CONDENSED ) family = "FiraSansCondensed";
	}

	// Handle Weight
	std::string weight = "Regular";
	Weight w = props.weight.value_or( Weight::REGULAR );
	switch ( w )
	{
		case Weight::LIGHT: weight = "Light"; break;
		case Weight::REGULAR: weight = "Regular"; break;
		case Weight::BOLD: weight = "Bold"; break;
		case Weight::BLACK: weight = "Heavy"; break;
	}

	// Handle Slant
	std::string slant = "";
	if ( props.slant == Slant::ITALIC || props.slant == Slant::OBLIQUE ) {
		// FiraCode and FiraMono don't seem to have italic files in the listing
		if ( family != "FiraCode" && family != "FiraMono" && family != "FiraMath" ) {
			slant = "Italic";
		}
	}

	std::string extension = ".ttf";
	if ( family == "FiraMath" || family == "FiraSans" || family.starts_with("FiraSans") ) {
		// Actually checking the previous ls output:
		// FiraMath-Regular.otf
		// FiraSans-Regular.ttf
		if ( family == "FiraMath" ) extension = ".otf";
	}

	std::string filename = family + "-" + weight + slant + extension;

	// Check if this specific combination exists (simplistic check or just return)
	// For now, I'll just return it.
	return filename;
}

std::size_t get_cjk_face_index( const FontProperties &props )
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

std::string get_devanagari_filename( const FontProperties &props )
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

FontStack create_font_stack( const FontStyle &style, Theme &theme, std::string_view base_name )
{
	auto properties = style.properties();
	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( get_fira_filename( properties, base_name ), 0, font_size ) );
	fonts.push( theme.load_font( get_devanagari_filename( properties ), 0, font_size ) );
	fonts.push( theme.load_font( "SourceHanSerif.ttc", get_cjk_face_index( properties ), font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}
}

FontStack intent_prose( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraSans" ); }
FontStack intent_code( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraCode" ); }
FontStack intent_math( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraMath" ); }
FontStack intent_title( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraSans" ); }
FontStack intent_label( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraGO" ); }
FontStack intent_annotation( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraSans" ); }
FontStack intent_caption( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraSansCondensed" ); }

Theme create_theme( void )
{
	Theme theme( "Fira" );
	theme.register_factory( "prose", intent_prose );
	theme.register_factory( "code", intent_code );
	theme.register_factory( "math", intent_math );
	theme.register_factory( "title", intent_title );
	theme.register_factory( "label", intent_label );
	theme.register_factory( "annotation", intent_annotation );
	theme.register_factory( "caption", intent_caption );
	return theme;
}

}
