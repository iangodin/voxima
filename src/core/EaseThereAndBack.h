#pragma once

#include "core/Ease.h"

// There and back: 0 at 0, out to 1 at the middle, and 0 again at 1, smoothly both ways, for a pulse. These are the eases that
// do not end at 1, so what they drive is left as it started.
float ease_there_and_back( float t );

// The same, staying at 1 for a part of the time: `pause` from 0 to 1 (a third, by default)
EaseFunction ease_there_and_back_with_pause( float pause = 1.F / 3.F );
