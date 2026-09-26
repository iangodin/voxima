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
			ThemeUtils::get_theme_font_size( properties );

		FontStack fonts;
		fonts.append( theme.load_font(
			ThemeUtils::get_newcm_filename( properties, base_name ), 0, font_size ) );
		fonts.append(
			theme.load_font( ThemeUtils::get_amiri_filename( properties ), 0, font_size ) );
		fonts.append( theme.load_font( "Symbola-Emoji.ttf", 0, font_size ) );
		fonts.append( theme.load_font( "SourceHanSerif.ttc", 0, font_size ) );
		fonts.append( theme.load_font( "NewCMMono10-Regular.otf", 0, font_size ) );
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

FontStack intent_math( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCMMath" );
}

FontStack intent_title( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM" );
}

FontStack intent_label( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM" );
}

FontStack intent_annotation( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM" );
}

FontStack intent_caption( const FontStyle &style, Theme &theme )
{
	return create_font_stack( style, theme, "NewCM" );
}

// A printed textbook's listing: warm, muted inks, keywords in bold and comments in italic
CodePalette code_palette()
{
	const auto rgb = Color::rgb;
	CodePalette palette;
	palette.line_highlight = Color::rgb( 214, 168, 108 );
	palette.line_highlight.a = 0.18F;
	palette.plain = { rgb( 232, 226, 208 ) };
	palette.set( "comment", { rgb( 140, 133, 119 ), std::nullopt, Slant::ITALIC } );
	palette.set( "keyword", { rgb( 215, 137, 127 ), Weight::BOLD } );
	palette.set( "string", { rgb( 156, 191, 138 ) } );
	palette.set( "number", { rgb( 214, 168, 108 ) } );
	palette.set( "constant", { rgb( 214, 168, 108 ) } );
	palette.set( "type", { rgb( 201, 179, 126 ) } );
	palette.set( "function", { rgb( 157, 180, 208 ) } );
	palette.set( "property", { rgb( 201, 160, 200 ) } );
	palette.set( "variable.builtin", { rgb( 215, 137, 127 ) } );
	palette.set( "label", { rgb( 201, 160, 200 ) } );
	return palette;
}

Theme create_theme( void )
{
	Theme theme( "Academic Classic" );
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
