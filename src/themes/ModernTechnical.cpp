#include <core/Theme.h>
#include <themes/ModernTechnical.h>
#include <themes/ThemeUtils.h>

#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

namespace ModernTechnical
{

namespace
{

	FontStack create_font_stack( const FontStyle &style, Theme &theme, std::string_view base_name )
	{
		auto properties = style.properties();
		const float font_size =
			ThemeUtils::get_theme_font_size( properties );

		FontStack fonts;
		fonts.append( theme.load_font(
			ThemeUtils::get_fira_filename( properties, base_name ), 0, font_size ) );
		fonts.append(
			theme.load_variable_font( "NotoEmoji-Variable.ttf", 0, font_size, 400, 100, 0 ) );
		fonts.append( theme.load_font( "NotoSansCJK-Variable.ttc", 0, font_size ) );
		fonts.append( theme.load_font(
			ThemeUtils::get_fira_filename( properties, "FiraMono" ), 0, font_size ) );
		fonts.append( theme.load_notdef_font( font_size ) );
		return fonts;
	}
}

FontStack intent_prose( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "FiraGO" );
}

FontStack intent_code( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "FiraMono" );
}

FontStack intent_math( const FontStyle &style, Theme &theme )
{
	auto properties = style.properties();
	const float font_size =
		ThemeUtils::get_theme_font_size( properties );

	FontStack fonts;
	fonts.append( theme.load_font( "FiraMath-Regular.otf", 0, font_size ) );
	fonts.append( theme.load_variable_font( "NotoEmoji-Variable.ttf", 0, font_size, 400, 100, 0 ) );
	fonts.append( theme.load_font( "NotoSansCJK-Variable.ttc", 0, font_size ) );
	fonts.append(
		theme.load_font( ThemeUtils::get_fira_filename( properties, "FiraMono" ), 0, font_size ) );
	fonts.append( theme.load_notdef_font( font_size ) );
	return fonts;
}

FontStack intent_title( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "FiraGO" );
}
FontStack intent_label( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "FiraGO" );
}
FontStack intent_annotation( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "FiraGO" );
}
FontStack intent_caption( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "FiraGO" );
}


Theme create_theme( void )
{
	Theme theme( "Modern Technical" );
	theme.set_palette( Palette::house( "github-dark" ) );
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
