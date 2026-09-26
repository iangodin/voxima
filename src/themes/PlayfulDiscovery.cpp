#include <core/Theme.h>
#include <themes/PlayfulDiscovery.h>
#include <themes/ThemeUtils.h>

#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

namespace PlayfulDiscovery
{

namespace
{
	FontStack create_font_stack( const FontStyle &style, Theme &theme )
	{
		auto properties = style.properties();
		const float font_size = ThemeUtils::get_theme_font_size( properties );
		auto params = ThemeUtils::get_font_params( properties );

		FontStack fonts;
		// Math: GFS Neohellenic Math, Main: Comfortaa (Variable), Arabic: Tajawal, Emoji: Noto Emoji, CJK: Kosugi Maru, Mono: Courier (Fallback)
		fonts.append( theme.load_variable_font( "Comfortaa-Variable.ttf", 0, font_size, params.weight, params.width, params.slant ) );
		fonts.append( theme.load_font( ThemeUtils::get_tajawal_filename( properties ), 0, font_size ) );
		fonts.append( theme.load_variable_font( "NotoEmoji-Variable.ttf", 0, font_size, params.weight, params.width, params.slant ) );
		fonts.append( theme.load_font( "KosugiMaru-Regular.ttf", 0, font_size ) );
		fonts.append( theme.load_font( "DejaVuSansMono.ttf", 0, font_size ) );
		return fonts;
	}
}

FontStack intent_prose( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme );
}

FontStack intent_code( const FontStyle &style, Theme &theme )
{
	// The monospaced font first, so code lines up in columns; the rest for what it lacks
	const auto properties = style.properties();
	FontStack fonts = create_font_stack( style, theme );
	fonts.prepend( theme.load_font( ThemeUtils::get_dejavusansmono_filename( properties ), 0, ThemeUtils::get_theme_font_size( properties ) ) );
	return fonts;
}

FontStack intent_math( const FontStyle &style, Theme &theme )
{
	auto properties = style.properties();
	const float font_size = ThemeUtils::get_theme_font_size( properties );

	FontStack fonts = create_font_stack( style, theme );
	fonts.prepend( theme.load_font( "GFSNeohellenicMath.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_title( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme );
}

FontStack intent_label( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme );
}

FontStack intent_annotation( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme );
}

FontStack intent_caption( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme );
}


Theme create_theme( void )
{
	Theme theme( "Playful Discovery" );
	theme.set_palette( Palette::house( "catppuccin-mocha" ) );
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
