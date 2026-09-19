#include "visuals/Row.h"
#include "visuals/Layout.h"
#include <algorithm>

namespace
{
constexpr int NONE = 0;
constexpr int THIN = 3;
constexpr int MEDIUM = 4;
constexpr int THICK = 5;

struct Space
{
	int mu = NONE;
	bool script_too = true; // false: only in text and display styles (TeX's parenthesized entries)
};

constexpr Space X = { NONE, true };  // cannot happen after the binary adjustment
constexpr Space S0 = { NONE, true };
constexpr Space T1 = { THIN, true };
constexpr Space T1c = { THIN, false };
constexpr Space M2c = { MEDIUM, false };
constexpr Space K3c = { THICK, false };

// TeXbook p. 170: rows are the left atom, columns the right atom
//                  Ord  Op   Bin  Rel  Open Close Punct Inner
constexpr Space TABLE[8][8] = {
	/* Ord   */ { S0,  T1,  M2c, K3c, S0,  S0,   S0,   T1c },
	/* Op    */ { T1,  T1,  X,   K3c, S0,  S0,   S0,   T1c },
	/* Bin   */ { M2c, M2c, X,   X,   M2c, X,    X,    M2c },
	/* Rel   */ { K3c, K3c, X,   S0,  K3c, S0,   S0,   K3c },
	/* Open  */ { S0,  S0,  X,   S0,  S0,  S0,   S0,   S0 },
	/* Close */ { S0,  T1,  M2c, K3c, S0,  S0,   S0,   T1c },
	/* Punct */ { T1c, T1c, X,   T1c, T1c, T1c,  T1c,  T1c },
	/* Inner */ { T1c, T1,  M2c, K3c, T1c, S0,   T1c,  T1c },
};

// TeX rules 5 and 6: a binary operator with nothing to operate on becomes ordinary
std::vector<AtomClass> effective_classes( const std::vector<AtomClass> &atoms )
{
	std::vector<AtomClass> eff = atoms;
	for ( size_t i = 0; i < eff.size(); ++i )
	{
		if ( eff[i] != AtomClass::BIN )
		{
			continue;
		}
		bool no_left = i == 0;
		if ( !no_left )
		{
			AtomClass prev = eff[i - 1];
			no_left = prev == AtomClass::BIN || prev == AtomClass::OP || prev == AtomClass::REL ||
			          prev == AtomClass::OPEN || prev == AtomClass::PUNCT;
		}
		bool no_right = i + 1 == eff.size();
		if ( !no_right )
		{
			AtomClass next = eff[i + 1];
			no_right = next == AtomClass::REL || next == AtomClass::CLOSE || next == AtomClass::PUNCT;
		}
		if ( no_left || no_right )
		{
			eff[i] = AtomClass::ORD;
		}
	}
	return eff;
}
}

Row::Row( const MathFont &font, float glyph_scale, ScriptLevel level )
	: _mu( glyph_scale * static_cast<float>( font.face().face->units_per_EM ) / 18.F ),
	  _level( level )
{
	math_axis = font.axis_height() * glyph_scale;
}

int Row::spacing_mu( AtomClass left, AtomClass right, ScriptLevel level )
{
	const Space space = TABLE[static_cast<size_t>( left )][static_cast<size_t>( right )];
	if ( !space.script_too && level != ScriptLevel::TEXT )
	{
		return NONE;
	}
	return space.mu;
}

Visual &Row::append( AtomClass atom, std::unique_ptr<Visual> &&visual )
{
	Visual &ref = add( std::move( visual ) );
	_atoms.push_back( atom );
	layout();
	return ref;
}

void Row::layout()
{
	const std::vector<AtomClass> eff = effective_classes( _atoms );

	float x = 0.F;
	for ( size_t i = 0; i < _visuals.size(); ++i )
	{
		Visual &child = *_visuals[i];
		if ( i > 0 )
		{
			x += static_cast<float>( spacing_mu( eff[i - 1], eff[i], _level ) ) * _mu;
		}

		// Left edge at x, baseline at y = 0
		const AABB box = layout_extent( child );
		child.position = child.position.movedBy( x - box.min.x(), -( child.position.y() + child.baseline ) );
		x += box.width();
	}

	// The box runs from the origin to the end of the last atom, and covers every child
	extent.reset();
	AABB box = layout_extent( *this );
	if ( !box.empty() )
	{
		box.min = { 0.F, box.min.y() };
		box.max = { x, box.max.y() };
		extent = box;
	}
}
