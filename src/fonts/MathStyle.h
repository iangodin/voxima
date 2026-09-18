#pragma once

#include "fonts/FontStyle.h"
#include "fonts/MathFont.h"

// How far a piece of math sits below body size (TeX's math styles).
enum class ScriptLevel
{
	TEXT,          // body size
	SCRIPT,        // superscripts, subscripts
	SCRIPT_SCRIPT, // second-level scripts, and the index of a root
};

// The font style for `level`, derived from the body style `base`. Sizes come from the
// font's own MATH constants (scriptPercentScaleDown, scriptScriptPercentScaleDown),
// which are both relative to the body size, so they aren't compounded.
inline FontStyle script_style( const FontStyle &base, const MathFont &font, ScriptLevel level )
{
	FontStyle style = base;
	switch ( level )
	{
		case ScriptLevel::TEXT:
			break;
		case ScriptLevel::SCRIPT:
			style.scaled( font.script_percent_scale_down() / 100.F );
			break;
		case ScriptLevel::SCRIPT_SCRIPT:
			style.scaled( font.script_script_percent_scale_down() / 100.F );
			break;
	}
	return style;
}
