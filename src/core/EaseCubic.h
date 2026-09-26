#pragma once

#include <cmath>

// t cubed: slow at first, then fast
float ease_cubic_in( float t );

// Fast at first, then slow
float ease_cubic_out( float t );

// Slow, fast, slow
float ease_cubic_in_out( float t );
