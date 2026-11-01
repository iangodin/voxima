#include "math/Context.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace math
{

Context::Metrics::Metrics( const FontFace &face )
	: font( face ), glyph_scale( face.scale / static_cast<float>( face.face->units_per_EM ) )
{
}

Context::Context( Theme &theme, FontStyle body ) : _theme( &theme ), _body( std::move( body ) ) {}

FontStyle Context::font_style( MathStyle style ) const
{
	// The script sizes are percentages of the body size given by the body-size font itself
	return script_style( _body, metrics( MathStyle::text() ).font, style );
}

const Context::Metrics &Context::metrics( MathStyle style ) const
{
	const ScriptLevel level = style.script_level();
	if ( auto found = _metrics.find( level ); found != _metrics.end() )
	{
		return *found->second;
	}

	// The body size needs no other font; the script sizes are taken from it
	const FontStyle font_style =
		level == ScriptLevel::TEXT ? _body : script_style( _body, metrics( MathStyle::text() ).font, level );
	const MathFonts fonts = _theme->get_math( font_style );
	return *( _metrics[level] = std::make_unique<Metrics>( fonts.roman() ) );
}

}
