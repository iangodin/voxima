#include <core/Theme.h>
#include <themes/EngineeringBlueprint.h>
#include <themes/ThemeUtils.h>

#include <fonts/FontStack.h>
#include <fonts/FontStyle.h>

namespace EngineeringBlueprint
{

namespace
{
	FontStack create_font_stack( const FontStyle &style, Theme &theme )
	{
		auto properties = style.properties();
		const float font_size = ThemeUtils::get_theme_font_size( properties );
		auto params = ThemeUtils::get_font_params( properties );

		FontStack fonts;
		fonts.append( theme.load_variable_font(
			"IBMPlexSans-Variable.ttf", 0, font_size, params.weight, params.width, params.slant ) );
		fonts.append( theme.load_font( ThemeUtils::get_ibmplexsansarabic_filename( properties ), 0, font_size ) );
		fonts.append( theme.load_variable_font(
			"NotoEmoji-Variable.ttf", 0, font_size, params.weight, params.width, params.slant ) );
		fonts.append( theme.load_font( ThemeUtils::get_ibmplexsansjp_filename( properties ), 0, font_size ) );
		fonts.append( theme.load_font( ThemeUtils::get_ibmplexmono_filename( properties ), 0, font_size ) );
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
	fonts.prepend( theme.load_font( ThemeUtils::get_ibmplexmono_filename( properties ), 0, ThemeUtils::get_theme_font_size( properties ) ) );
	return fonts;
}

FontStack intent_math( const FontStyle &style, Theme &theme )
{
	auto properties = style.properties();
	const float font_size = ThemeUtils::get_theme_font_size( properties );

	FontStack fonts = create_font_stack( style, theme );
	fonts.prepend( theme.load_font( "STIX2Math.otf", 0, font_size ) );
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

// A blueprint: cyan and white on the dark, with amber for literals like the notes on a drawing
CodePalette code_palette()
{
	const auto rgb = Color::rgb;
	CodePalette palette;
	palette.plain = { rgb( 214, 228, 240 ) };
	palette.set( "comment", { rgb( 106, 139, 168 ), std::nullopt, Slant::ITALIC } );
	palette.set( "keyword", { rgb( 79, 195, 247 ) } );
	palette.set( "string", { rgb( 255, 213, 79 ) } );
	palette.set( "number", { rgb( 255, 171, 64 ) } );
	palette.set( "constant", { rgb( 255, 171, 64 ) } );
	palette.set( "type", { rgb( 128, 203, 196 ) } );
	palette.set( "function", { rgb( 255, 255, 255 ), Weight::BOLD } );
	palette.set( "property", { rgb( 179, 229, 252 ) } );
	palette.set( "operator", { rgb( 77, 208, 225 ) } );
	palette.set( "variable.builtin", { rgb( 79, 195, 247 ) } );
	palette.set( "label", { rgb( 255, 171, 64 ) } );
	return palette;
}

Theme create_theme( void )
{
	Theme theme( "Engineering Blueprint" );
	theme.set_code_palette( code_palette() );
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
