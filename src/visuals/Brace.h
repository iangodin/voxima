#pragma once

#include "core/Theme.h"
#include "visuals/Group.h"
#include "visuals/CreateText.h"
#include "visuals/Path.h"
#include "visuals/Placement.h"

#include <memory>
#include <string>

struct BraceOptions
{
	// How far the brace's middle point stands out from the line it spans (stage units); the curls at its ends are half that
	float depth = 0.5F;
	float thickness = 0.04F;
	Color color = { 1.F, 1.F, 1.F, 1.F };

	// Which side of the line from `from` to `to` it bulges to: true for the left (looking from `from` toward `to`)
	bool left = true;

	// Between the brace's point and its label (stage units)
	float label_gap = 0.2F;
};

// A curly brace spanning the line from one point to another, its point (the middle) standing out to one side
std::unique_ptr<Path> make_brace( Point from, Point to, const BraceOptions &options = {} );

// A brace along one side of a visual's box (LEFT, RIGHT, TOP or BOTTOM), a `gap` out from it, its point away from the visual
std::unique_ptr<Path> make_brace_beside( const Visual &visual, Anchor side, float gap = 0.15F, const BraceOptions &options = {} );

// A brace with a label at its point: a Group of the two (child 0 the brace, child 1 the label). The label's middle is on the
// line out from the brace's point, and its near edge a label_gap from it.
std::unique_ptr<Group> make_labeled_brace( Theme &theme, Point from, Point to, const std::string &label, const FontStyle &style, const BraceOptions &options = {} );

// The same along a side of a visual
std::unique_ptr<Group> make_labeled_brace_beside( Theme &theme, const Visual &visual, Anchor side, const std::string &label, const FontStyle &style, float gap = 0.15F, const BraceOptions &options = {} );
