#include "visuals/LargeOperator.h"
#include "visuals/Tags.h"
#include "visuals/CreateMathLargeOperation.h"
#include "visuals/Limits.h"
#include "visuals/Scripts.h"

bool is_integral( char32_t op )
{
	return ( op >= 0x222B && op <= 0x2233 ) || ( op >= 0x2A0B && op <= 0x2A1C );
}

LimitPlacement default_limit_placement( char32_t op )
{
	return is_integral( op ) ? LimitPlacement::SIDE : LimitPlacement::OVER_UNDER;
}

namespace
{
std::unique_ptr<Group> attach_limits(
	const MathFont &font,
	float glyph_scale,
	std::unique_ptr<Visual> &&nucleus,
	std::unique_ptr<Visual> &&upper,
	std::unique_ptr<Visual> &&lower,
	bool over_under )
{
	// Either form names its limits the same way
	if ( upper )
	{
		tag_visual( *upper, "upper" );
	}
	if ( lower )
	{
		tag_visual( *lower, "lower" );
	}

	if ( over_under )
	{
		return std::make_unique<Limits>(
			font, glyph_scale, std::move( nucleus ), std::move( upper ), std::move( lower ) );
	}
	return std::make_unique<Scripts>(
		font, glyph_scale, std::move( nucleus ), std::move( upper ), std::move( lower ) );
}
}

std::unique_ptr<Group> make_large_operator(
	const MathFont &font,
	float glyph_scale,
	char32_t op,
	std::unique_ptr<Visual> &&upper,
	std::unique_ptr<Visual> &&lower,
	MathStyle style,
	std::optional<LimitPlacement> placement )
{
	const bool display = style.is_display();
	// create_math_large_op works in font units and takes the height it should reach
	auto nucleus = create_math_large_op( font, op, display ? font.display_operator_min_height() : 0.F );
	*nucleus *= glyph_scale;

	const LimitPlacement where = placement.value_or( default_limit_placement( op ) );
	return attach_limits(
		font, glyph_scale, std::move( nucleus ), std::move( upper ), std::move( lower ),
		display && where == LimitPlacement::OVER_UNDER );
}

std::unique_ptr<Group> make_named_operator(
	const MathFont &font,
	float glyph_scale,
	std::unique_ptr<Visual> &&nucleus,
	std::unique_ptr<Visual> &&upper,
	std::unique_ptr<Visual> &&lower,
	MathStyle style,
	bool moving_limits )
{
	return attach_limits(
		font, glyph_scale, std::move( nucleus ), std::move( upper ), std::move( lower ),
		moving_limits && style.is_display() );
}
