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

	switch ( properties.scale.value_or( Scale::NORMAL ) )
	{
		case Scale::TINY:
			fonts.push( theme.load_font( "NewCM08-Regular.otf", 0, 0.2F ) );
			break;

		case Scale::SMALL:
			fonts.push( theme.load_font( "NewCM08-Regular.otf", 0, 0.25F ) );
			break;

		case Scale::NORMAL:
			fonts.push( theme.load_font( "NewCM08-Regular.otf", 0, 0.3F ) );
			break;

		case Scale::LARGE:
			fonts.push( theme.load_font( "NewCM08-Regular.otf", 0, 0.75 ) );
			break;

		case Scale::HUGE:
			fonts.push( theme.load_font( "NewCM08-Regular.otf", 0, 1.5 ) );
			break;
	}

	fonts.push( theme.load_font( "AND-Regular.otf", 0, 0.2F ) );
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
