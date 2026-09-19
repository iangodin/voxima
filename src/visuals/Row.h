#pragma once

#include "visuals/Group.h"
#include "visuals/Visual.h"
#include "fonts/MathFont.h"
#include "fonts/MathStyle.h"
#include <memory>
#include <vector>

// TeX's math atom classes: they decide the space put between neighbours.
enum class AtomClass
{
	ORD,   // variables, digits
	OP,    // large operators, function names (sin, lim)
	BIN,   // + - x
	REL,   // = < >
	OPEN,  // ( [ {
	CLOSE, // ) ] }
	PUNCT, // , ;
	INNER, // fractions, delimited groups
};

// A horizontal row of math atoms: children sit left to right on a common baseline (y = 0),
// separated by TeX's inter-atom space (thin, medium and thick = 3, 4 and 5 mu, 18 mu being
// one em). A binary operator that has no left operand or no right operand (as in "-x"
// or "a + = b") is treated as ordinary, as TeX does. In script styles only the thin space
// remains.
//
// The row lays itself out again on every append(). Add children with append(), not add().
class Row : public Group
{
public:
	Row( const MathFont &font, float glyph_scale, ScriptLevel level = ScriptLevel::TEXT );

	Visual &append( AtomClass atom, std::unique_ptr<Visual> &&visual );

	// Extra space (in mu) before the next atom, on top of the automatic spacing, like
	// TeX's \, \: \; and \! (which is negative). It is glue, not an atom: it doesn't change
	// how the atoms around it are classified. Space left at the end widens the row.
	static constexpr float THIN = 3.F;
	static constexpr float MEDIUM = 4.F;
	static constexpr float THICK = 5.F;
	static constexpr float QUAD = 18.F;
	void append_space( float mu );

	// The space TeX puts between two atoms, in mu (0, 3, 4 or 5), after any binary
	// operator adjustment has been applied to `left` and `right`.
	static int spacing_mu( AtomClass left, AtomClass right, ScriptLevel level );

private:
	void layout();

	float _mu = 0.F;
	ScriptLevel _level = ScriptLevel::TEXT;
	std::vector<AtomClass> _atoms;
	std::vector<float> _extra_mu; // explicit space before each atom
	float _pending_mu = 0.F;      // explicit space waiting for the next atom
};
