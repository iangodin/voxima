#include <core/Theme.h>

#include <themes/PlayfulDiscovery.h>

#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

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

FontStack create_font_stack( const FontStyle &style, Theme &theme )
{
	auto properties = style.properties();
	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	int weight = 400; // Regular
	if ( properties.weight == Weight::LIGHT ) weight = 300;
	if ( properties.weight == Weight::BOLD ) weight = 700;
	if ( properties.weight == Weight::BLACK ) weight = 900;

	int width = 100; // Normal
	int slant = (properties.slant == Slant::ITALIC || properties.slant == Slant::OBLIQUE) ? 1 : 0;

	FontStack fonts;
	// Math: GFS Neohellenic Math, Main: Comfortaa (Variable), Arabic: Tajawal, Emoji: Noto Emoji, CJK: Kosugi Maru, Mono: Courier (Fallback)
	fonts.push( theme.load_font( "GFSNeohellenicMath.otf", 0, font_size ) );
	fonts.push( theme.load_variable_font( "Comfortaa-Variable.ttf", 0, font_size, weight, width, slant ) );
	fonts.push( theme.load_font( "Tajawal-Regular.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "NotoEmoji-Variable.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "KosugiMaru-Regular.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "DejaVuSansMono.ttf", 0, font_size ) );
	return fonts;
}
}

FontStack intent_prose( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme ); }
FontStack intent_code( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme ); }
FontStack intent_math( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme ); }
FontStack intent_title( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme ); }
FontStack intent_label( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme ); }
FontStack intent_annotation( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme ); }
FontStack intent_caption( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme ); }

Theme create_theme( void )
{
	Theme theme( "Playful Discovery" );
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
