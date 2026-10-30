#pragma once

#include <fonts/FontStyle.h>

namespace ThemeUtils
{

// How big text is, in stage units: the scale's size (tiny to huge), times the style's own factor. The same for every
// theme: what differs between themes is which files the fonts are (see FontFamily and themes/*.huml).
float get_theme_font_size( Scale scale );
float get_theme_font_size( const FontProperties &props );

}
