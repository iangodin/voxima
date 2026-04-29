#include "NewComputerModern.h"

#include "Theme.h"

namespace NewComputerModern
{

Theme create_theme( void )
{
	Theme theme;
	theme.register_factory( "prose", intent_prose );
	theme.register_factory( "math", intent_math );
	return theme;
}

FontStack intent_prose( const TextStyle &style, Theme &theme )
{
	auto properties = style.properties();

	FontStack fonts;

	float font_size = 0.3F;

	switch ( properties.scale.value_or( Scale::NORMAL ) )
	{
		case Scale::TINY: font_size = 0.1F; break;
		case Scale::SMALL: font_size = 0.25F; break;
		case Scale::NORMAL: font_size = 0.3F; break;
		case Scale::LARGE: font_size = 0.75F; break;
		case Scale::HUGE: font_size = 1.5F; break;
	}

	fonts.push( theme.load_font( "NewCM08-Regular.otf", 0, font_size ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, font_size ) );
	return fonts;
}

FontStack intent_math( [[maybe_unused]] const TextStyle &style, Theme &theme )
{
	FontStack fonts;
	fonts.push( theme.load_font( "NewCMMath-Regular.otf", 0, 0.2F ) );
	fonts.push( theme.load_font( "AND-Regular.otf", 0, 0.2F ) );
	return fonts;
}

}
