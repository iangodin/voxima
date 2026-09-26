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

// Clean and modern, in the colors of GitHub's dark theme. Fira Mono has no italic, so comments are only a color.
CodePalette code_palette()
{
	const auto rgb = Color::rgb;
	CodePalette palette;
	palette.line_highlight = Color::rgb( 56, 139, 253 );
	palette.line_highlight.a = 0.18F;
	palette.plain = { rgb( 230, 237, 243 ) };
	palette.set( "comment", { rgb( 139, 148, 158 ) } );
	palette.set( "keyword", { rgb( 255, 123, 114 ) } );
	palette.set( "string", { rgb( 165, 214, 255 ) } );
	palette.set( "number", { rgb( 121, 192, 255 ) } );
	palette.set( "constant", { rgb( 121, 192, 255 ) } );
	palette.set( "type", { rgb( 255, 166, 87 ) } );
	palette.set( "function", { rgb( 210, 168, 255 ) } );
	palette.set( "property", { rgb( 126, 231, 135 ) } );
	palette.set( "operator", { rgb( 255, 123, 114 ) } );
	palette.set( "variable.builtin", { rgb( 121, 192, 255 ) } );
	palette.set( "label", { rgb( 255, 166, 87 ) } );
	return palette;
}

Theme create_theme( void )
{
	Theme theme( "Modern Technical" );
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
