#pragma once

#include "core/Theme.h"
#include "fonts/FontStyle.h"
#include "fonts/MathFont.h"
#include "fonts/MathStyle.h"

#include <map>
#include <memory>

namespace math
{

// What formulas are built from: the theme, and the style of body-size text (its size sets the
// size of the whole formula). From these it gives the font and text style to use in any math
// style, so nothing else needs to know the sizes of scripts and the like.
class Context
{
public:
	// `body` must use the math intent. The default is large, for display on a slide.
	explicit Context( Theme &theme, FontStyle body = FontStyle{ Intent::MATH }.large() );

	[[nodiscard]] Theme &theme() const { return *_theme; }

	// The style text is set in for `style`: the body style at that style's size
	[[nodiscard]] FontStyle font_style( MathStyle style ) const;

	// The math font at the size of `style`, and the scale from its font units to canvas units:
	// what the composites (Fraction, Radical ...) are constructed with
	struct Metrics
	{
		MathFont font;
		float glyph_scale;
		explicit Metrics( const FontFace &face );
	};
	[[nodiscard]] const Metrics &metrics( MathStyle style ) const;

private:
	Theme *_theme;
	FontStyle _body;
	mutable std::map<ScriptLevel, std::unique_ptr<Metrics>> _metrics;
};

}
