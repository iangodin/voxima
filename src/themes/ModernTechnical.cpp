#include <core/Theme.h>

#include <themes/ModernTechnical.h>

#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

namespace ModernTechnical
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

FontStack create_font_stack( const FontStyle &style, Theme &theme, std::string_view base_name )
{
	auto properties = style.properties();
	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	std::string weight = "Regular";
	if ( properties.weight == Weight::LIGHT ) weight = "Light";
	if ( properties.weight == Weight::BOLD ) weight = "Bold";
	if ( properties.weight == Weight::BLACK ) weight = "Heavy";

	FontStack fonts;
	// Main: FiraGO, Emoji: Noto Emoji, CJK: Noto Sans CJK, Mono: Fira Mono
	fonts.push( theme.load_font( std::string(base_name) + "-" + weight + ".ttf", 0, font_size ) );
	fonts.push( theme.load_font( "NotoEmoji-Variable.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "NotoSansCJK-Variable.otc", 0, font_size ) );
	fonts.push( theme.load_font( "FiraMono-Regular.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
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
	const float font_size = theme_font_size( properties.scale.value_or( Scale::MEDIUM ) );

	FontStack fonts;
	fonts.push( theme.load_font( "FiraMath-Regular.otf", 0, font_size ) );
	fonts.push( theme.load_font( "NotoEmoji-Variable.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "NotoSansCJK-Variable.otc", 0, font_size ) );
	fonts.push( theme.load_font( "FiraMono-Regular.ttf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_title( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraGO" ); }
FontStack intent_label( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraGO" ); }
FontStack intent_annotation( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraGO" ); }
FontStack intent_caption( const FontStyle &style, Theme &theme ) { return create_font_stack( style, theme, "FiraGO" ); }

Theme create_theme( void )
{
	Theme theme( "Modern Technical" );
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
