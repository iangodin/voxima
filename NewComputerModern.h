#pragma once

#include "Theme.h"
#include "TextStyle.h"
#include "FontStack.h"

namespace NewComputerModern
{

Theme create_theme( void );

FontStack intent_prose( const TextStyle &style, Theme &theme );
FontStack intent_math( const TextStyle &style, Theme &theme );

}
