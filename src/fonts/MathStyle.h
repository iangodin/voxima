#pragma once

#include "fonts/FontStyle.h"
#include "fonts/MathFont.h"

// How far a piece of math sits below body size.
enum class ScriptLevel
{
	TEXT,          // body size
	SCRIPT,        // superscripts, subscripts
	SCRIPT_SCRIPT, // second-level scripts, and the index of a root
};

// TeX's math styles: a level, and whether the content is "cramped" (under something that
// shouldn't have superscripts rising into it: a radical, a denominator, a subscript). The
// level picks the size and, for display, the more open constants of the font; cramped only
// lowers superscripts (SuperscriptShiftUpCramped). The functions below give the style of a
// part of a formula from the style of the whole, TeX's rules for this.
struct MathStyle
{
	enum class Level
	{
		DISPLAY, // a formula on its own line: body size, open spacing
		TEXT,    // a formula inside running text: body size, tighter
		SCRIPT,
		SCRIPT_SCRIPT,
	};

	Level level = Level::DISPLAY;
	bool cramped = false;

	MathStyle() = default;
	MathStyle( Level level_, bool cramped_ = false ) : level( level_ ), cramped( cramped_ ) {}

	// A size only: text style for the body, script for a script, and so on
	MathStyle( ScriptLevel size ) // NOLINT: implicit on purpose
	{
		switch ( size )
		{
			case ScriptLevel::TEXT:
				level = Level::TEXT;
				break;
			case ScriptLevel::SCRIPT:
				level = Level::SCRIPT;
				break;
			case ScriptLevel::SCRIPT_SCRIPT:
				level = Level::SCRIPT_SCRIPT;
				break;
		}
	}

	static MathStyle display() { return { Level::DISPLAY }; }
	static MathStyle text() { return { Level::TEXT }; }
	static MathStyle script() { return { Level::SCRIPT }; }
	static MathStyle script_script() { return { Level::SCRIPT_SCRIPT }; }

	[[nodiscard]] bool is_display() const { return level == Level::DISPLAY; }

	// The size this style is set at
	[[nodiscard]] ScriptLevel script_level() const
	{
		switch ( level )
		{
			case Level::SCRIPT:
				return ScriptLevel::SCRIPT;
			case Level::SCRIPT_SCRIPT:
				return ScriptLevel::SCRIPT_SCRIPT;
			default:
				return ScriptLevel::TEXT;
		}
	}

	[[nodiscard]] MathStyle cramp() const { return { level, true }; }

	// The parts of a formula:
	// the numerator of a fraction is a step smaller, the denominator the same but cramped
	[[nodiscard]] MathStyle numerator() const { return { step_down( level ), cramped }; }
	[[nodiscard]] MathStyle denominator() const { return { step_down( level ), true }; }
	// a superscript is script size, a subscript the same but cramped
	[[nodiscard]] MathStyle superscript() const { return { script_of( level ), cramped }; }
	[[nodiscard]] MathStyle subscript() const { return { script_of( level ), true }; }
	// under a radical, overline or accent the content is cramped
	[[nodiscard]] MathStyle radicand() const { return cramp(); }
	// the index of a root is scriptscript
	[[nodiscard]] MathStyle index() const { return { Level::SCRIPT_SCRIPT, false }; }
	// cells of a matrix are text style when the matrix is displayed
	[[nodiscard]] MathStyle cell() const { return { is_display() ? Level::TEXT : level, cramped }; }

	bool operator==( const MathStyle &other ) const = default;

private:
	// display -> text -> script -> scriptscript
	static Level step_down( Level level )
	{
		switch ( level )
		{
			case Level::DISPLAY:
				return Level::TEXT;
			case Level::TEXT:
				return Level::SCRIPT;
			default:
				return Level::SCRIPT_SCRIPT;
		}
	}

	// display and text -> script; scripts -> scriptscript
	static Level script_of( Level level )
	{
		return ( level == Level::DISPLAY || level == Level::TEXT ) ? Level::SCRIPT : Level::SCRIPT_SCRIPT;
	}
};

// The font style for a size, derived from the body style `base`. Sizes come from the font's
// own MATH constants (scriptPercentScaleDown, scriptScriptPercentScaleDown), which are both
// relative to the body size, so they aren't compounded.
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

inline FontStyle script_style( const FontStyle &base, const MathFont &font, MathStyle style )
{
	return script_style( base, font, style.script_level() );
}
