#include <themes/ThemeUtils.h>

namespace ThemeUtils
{

float get_theme_font_size( Scale scale )
{
	switch ( scale )
	{
		case Scale::TINY:
			return 0.1F;
		case Scale::SMALL:
			return 0.25F;
		case Scale::MEDIUM:
			return 0.3F;
		case Scale::LARGE:
			return 0.75F;
		case Scale::HUGE:
			return 1.5F;
		default:
			return 0.3F;
	}
}

float get_theme_font_size( const FontProperties &props )
{
	return get_theme_font_size( props.scale.value_or( Scale::MEDIUM ) ) * props.size_factor.value_or( 1.F );
}

}
