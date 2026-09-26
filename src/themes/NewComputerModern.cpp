#include <themes/NewComputerModern.h>
#include <core/Theme.h>
#include <themes/ThemeUtils.h>

#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

namespace NewComputerModern
{

namespace
{
	FontStack create_font_stack( const FontStyle &style, Theme &theme, std::string_view base_name )
	{
		auto properties = style.properties();
		const float font_size = ThemeUtils::get_theme_font_size( properties );

		FontStack fonts;
		fonts.append( theme.load_font( ThemeUtils::get_newcm_filename( properties, base_name ), 0, font_size ) );
		fonts.append( theme.load_font( ThemeUtils::get_devanagari_filename( properties ), 0, font_size ) );
		fonts.append( theme.load_font( ThemeUtils::get_amiri_filename( properties ), 0, font_size ) );
		fonts.append( theme.load_font( "SourceHanSerif.ttc", ThemeUtils::get_cjk_face_index( properties ), font_size ) );
		fonts.append( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
		fonts.append( theme.load_notdef_font( font_size ) );
		return fonts;
	}
}

FontStack intent_prose( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM" );
}

FontStack intent_code( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCMMono" );
}

FontStack intent_title( const FontStyle &style, Theme &theme )
{
	FontStack fonts = create_font_stack( style, theme, "NewCMUncial" );
	return fonts;
}

FontStack intent_annotation( const FontStyle &style, Theme &theme )
{
	FontStack fonts = create_font_stack( style, theme, "NewCM" );
	return fonts;
}

FontStack intent_label( const FontStyle &style, Theme &theme )
{
	FontStack fonts = create_font_stack( style, theme, "NewCMSans" );
	return fonts;
}

FontStack intent_math( const FontStyle &style, Theme &theme )
{
	auto properties = style.properties();
	const float font_size = ThemeUtils::get_theme_font_size( properties );

	FontStack fonts = create_font_stack( style, theme, "NewCMMath" );
	fonts.prepend( theme.load_font( ThemeUtils::get_newcm_filename( properties, "NewCMMath" ), 0, font_size ) );
	return fonts;
}

FontStack intent_caption( const FontStyle &style, Theme &theme )
{
	FontStack fonts = create_font_stack( style, theme, "NewCM" );
	return fonts;
}

// One Dark's colors (see CodePalette::dark)
CodePalette code_palette()
{
	return CodePalette::dark();
}

Theme create_theme( void )
{
	Theme theme( "New Computer Modern" );
	theme.set_code_palette( code_palette() );
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
