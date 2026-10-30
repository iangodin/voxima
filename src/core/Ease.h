#pragma once

#include <functional>

// An ease turns how far along something is, t from 0 (the start) to 1 (the end), into how far along it looks:
// slower at first, overshooting, bouncing ... It is a plain function of t (so any moment can be worked out, in
// any order), 0 at 0 and 1 at 1, though in between it may go outside that (back and elastic eases overshoot).
// ease_cubic_in_out, ease_elastic_out ... are such functions; use them with tween() (Tween.h), or call them:
//
//     float x = start + ( end - start ) * ease_cubic_out( t );
//
// This is the type to keep one in, when it is chosen at run time.
using EaseFunction = std::function<float( float )>;
