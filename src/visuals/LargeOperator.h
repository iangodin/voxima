#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include "fonts/MathStyle.h"
#include <memory>
#include <optional>

enum class LimitPlacement
{
	OVER_UNDER, // above and below, centered (sums, products, lim)
	SIDE,       // as a superscript and subscript (integrals)
};

// Integral signs (∫ ∬ ∭ ∮ ...)
bool is_integral( char32_t op );

// TeX's default: integrals put their limits at the side, everything else over and under.
LimitPlacement default_limit_placement( char32_t op );

// A large operator (∑ ∏ ∫ ...) with an optional upper and lower limit. In display style the
// operator is drawn at least DisplayOperatorMinHeight tall and limits follow `placement`
// (the operator's default if not given). In text style the operator is its normal size
// and limits always go to the side. Limits are expected at script size. Returns a Limits
// (over/under) or a Scripts (side).
std::unique_ptr<Group> make_large_operator(
	const MathFont &font,
	float glyph_scale,
	char32_t op,
	std::unique_ptr<Visual> &&upper,
	std::unique_ptr<Visual> &&lower,
	MathStyle style = MathStyle::display(),
	std::optional<LimitPlacement> placement = std::nullopt );
