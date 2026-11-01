#include "visuals/Delimited.h"
#include "visuals/Tags.h"
#include "visuals/CreateMathLargeOperation.h"
#include "visuals/Fraction.h"
#include "visuals/MathMetrics.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <algorithm>

namespace
{
// TeX's \delimiterfactor (901/1000) and \delimitershortfall (5pt at 10pt, half an em)
constexpr float DELIMITER_FACTOR = 0.901F;
constexpr float DELIMITER_SHORTFALL_EM = 0.5F;
}

Delimited::Delimited(
	const MathFont &font,
	float s,
	char32_t open,
	std::unique_ptr<Visual> &&content,
	char32_t close,
	MathStyle style )
	: Row( font, s, style )
{
	const float em = s * static_cast<float>( font.face().face->units_per_EM );
	const float axis = font.axis_height() * s;

	// How far the content reaches from the math axis, above or below
	const ScriptBox box = measure_box( *content );
	const float delta = std::max( box.height() - axis, box.depth() + axis );
	const float target = std::max( DELIMITER_FACTOR * 2.F * delta, 2.F * delta - DELIMITER_SHORTFALL_EM * em );

	auto make_delimiter = [&]( char32_t code )
	{
		// create_math_large_op works in font units
		auto delimiter = create_math_large_op( font, code, target / s );
		*delimiter *= s;
		return delimiter;
	};

	if ( open )
	{
		tag_visual( append( AtomClass::OPEN, make_delimiter( open ) ), "open" );
	}
	tag_visual( append( AtomClass::ORD, std::move( content ) ), "content" );
	if ( close )
	{
		tag_visual( append( AtomClass::CLOSE, make_delimiter( close ) ), "close" );
	}
}

std::unique_ptr<Group> make_binomial(
	const MathFont &font,
	float glyph_scale,
	std::unique_ptr<Visual> &&top,
	std::unique_ptr<Visual> &&bottom,
	MathStyle style )
{
	// The parentheses are the delimiters, so the stack needs no null-delimiter space of its own
	auto stack = std::make_unique<Fraction>(
		font, glyph_scale, std::move( top ), std::move( bottom ), FractionOptions{ .rule = false, .side_space = false, .style = style } );
	return std::make_unique<Delimited>( font, glyph_scale, U'(', std::move( stack ), U')', style );
}
