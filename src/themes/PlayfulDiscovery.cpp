#include <core/Theme.h>

#include <themes/PlayfulDiscovery.h>

#include <fonts/FontStack.h>
#include <fonts/TextStyle.h>

namespace PlayfulDiscovery
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

FontStack create_font_stack( const TextStyle &style, Theme &theme, std::string_view base_name )
{
	auto properties = style.properties();
	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	// Main: Comfortaa, Math: GFS Neohellenic Math, Emoji: Noto Emoji, CJK: Kosugi Maru, Mono: Courier (Fallback)
	fonts.push( theme.load_font( std::string(base_name) + ".ttf", 0, font_size ) );
	fonts.push( theme.load_font( "GFSNeohellenicMath.otf", 0, font_size ) );
	fonts.push( theme.load_font( "NotoEmoji.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "KosugiMaru-Regular.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "Courier.ttf", 0, font_size ) );
	return fonts;
}
}

FontStack intent_prose( const TextStyle &style, Theme &theme ) { return create_font_stack( style, theme, "Comfortaa-Regular" ); }
FontStack intent_code( const TextStyle &style, Theme &theme ) { return create_font_stack( style, theme, "Courier" ); }
FontStack intent_math( const TextStyle &style, Theme &theme ) { return create_font_stack( style, theme, "Comfortaa-Regular" ); }
FontStack intent_title( const TextStyle &style, Theme &theme ) { return create_font_stack( style, theme, "Comfortaa-Bold" ); }
FontStack intent_label( const TextStyle &style, Theme &theme ) { return create_font_stack( style, theme, "Comfortaa-Regular" ); }
FontStack intent_annotation( const TextStyle &style, Theme &theme ) { return create_font_stack( style, theme, "Comfortaa-Regular" ); }
FontStack intent_caption( const TextStyle &style, Theme &theme ) { return create_font_stack( style, theme, "Comfortaa-Regular" ); }

Theme create_theme( void )
{
	Theme theme;
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
