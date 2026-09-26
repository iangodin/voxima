#pragma once

#include "visuals/Outline.h"
#include "visuals/Path.h"

#include <vector>

// The first `fraction` (0 to 1) of a contour's length, as path commands: the way to draw a shape on. Lengths are worked out
// from the curves' control points, close enough that a line is drawn at an even speed. A closed contour that is drawn in
// full is closed (its last corner joined, not capped).
std::vector<PathCommand> trimmed_commands( const Contour &contour, float fraction );

// The length of a contour (as trimmed_commands measures it)
float contour_length( const Contour &contour );
