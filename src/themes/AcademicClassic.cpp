#include <core/Theme.h>
#include <themes/AcademicClassic.h>
#include <themes/ThemeUtils.h>

#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

namespace AcademicClassic
{

namespace
{
	FontStack create_font_stack( const FontStyle &style, Theme &theme, std::string_view base_name )
	{
		auto properties = style.properties();
		const float font_size =
			ThemeUtils::get_theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

		FontStack fonts;
		fonts.append( theme.load_font(
			ThemeUtils::get_newcm_filename( properties, base_name ), 0, font_size ) );
		fonts.append(
			theme.load_font( ThemeUtils::get_amiri_filename( properties ), 0, font_size ) );
		fonts.append( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
		fonts.append( theme.load_font( "SourceHanSerif.ttc", 0, font_size ) );
		fonts.append( theme.load_font( "NewCMMono10-Regular.otf", 0, font_size ) );
		fonts.append( theme.load_font( "AND-Regular.otf", 0, font_size ) );
		return fonts;
	}
}

FontStack intent_prose( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM10-Regular" );
}

FontStack intent_code( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCMMono10-Regular" );
}

FontStack intent_math( const FontStyle &style, Theme &theme )
{
	auto properties = style.properties();
	const float font_size =
		ThemeUtils::get_theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts = create_font_stack( style, theme, "NewCMMath-Regular" );
	fonts.prepend( theme.load_font( "NewCMMath-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_title( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM10-Bold" );
}

FontStack intent_label( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM10-Regular" );
}

FontStack intent_annotation( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM10-Regular" );
}

FontStack intent_caption( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM10-Regular" );
}

Theme create_theme( void )
{
	Theme theme( "Academic Classic" );
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
