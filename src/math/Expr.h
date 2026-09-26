#pragma once

#include "math/Context.h"
#include "visuals/LargeOperator.h"
#include "visuals/Matrix.h"
#include "visuals/Row.h"
#include "visuals/Visual.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

// Formulas built as descriptions, then turned into visuals from the top down:
//
//     auto formula = math::radical( math::fraction( math::text( "1" ), math::text( "2" ) ) );
//     auto visual = formula.build( context );          // display style
//
// Nothing is laid out until build(), which passes each part its own style (the numerator of a
// display fraction is text style, a superscript is script size, a radicand is cramped, ...).
// So the parts don't take a style, and the sizes and constants come out right on their own.
// An Expr is cheap to copy and can be built any number of times, in any style.
namespace math
{

class Expr
{
public:
	using Builder = std::function<std::unique_ptr<Visual>( const Context &, MathStyle )>;

	// An empty expression: an optional part that isn't there (no superscript, no lower limit)
	Expr() = default;

	// An expression made by a function that gets the context and its style. The escape hatch for
	// anything that isn't built in.
	static Expr from( Builder builder );

	explicit operator bool() const { return _builder != nullptr; }

	// Lay the formula out. Display style unless told otherwise. Throws if empty.
	[[nodiscard]] std::unique_ptr<Visual> build( const Context &context, MathStyle style = MathStyle::display() ) const;

private:
	std::shared_ptr<const Builder> _builder;
};

// ---- Text --------------------------------------------------------------------------------

// Letters in math italic (the math intent's default), digits and symbols as they are
Expr text( std::string characters );
// No italic: function names, units, the d of dx
Expr upright( std::string characters );
Expr bold( std::string characters );
// Words between the math (TeX's \text{if}, \text{for all}): upright, the spaces kept. Put it in a row as an atom of its own
// class, e.g. ord( words( "if" ) ), with space( Row::QUAD ) round it as TeX's authors do.
Expr words( std::string characters );
// The rest of the Unicode math alphabets (\mathcal, \mathfrak, \mathbb, \mathsf, \mathtt),
// each set upright, as text() sets math italic
Expr script( std::string characters );
Expr fraktur( std::string characters );
Expr double_struck( std::string characters );
Expr sans( std::string characters );
Expr mono( std::string characters );

// ---- Constructs --------------------------------------------------------------------------

Expr fraction( Expr numerator, Expr denominator );
Expr binomial( Expr top, Expr bottom );

Expr radical( Expr radicand );
Expr root( Expr index, Expr radicand );

Expr superscript( Expr base, Expr script );
Expr subscript( Expr base, Expr script );
// Either script may be empty
// Primes after a symbol: f', f'' (one prime mark for each of `count`, as a superscript)
Expr primed( Expr base, int count = 1 );
Expr scripts( Expr base, Expr superscript_, Expr subscript_ );

// A label stacked over (or under) a base, centered on it and at script size, always -
// whatever the style, unlike a large operator's limits, which only stack in display style.
// The base keeps its own size and class; wrap the whole thing (e.g. rel( overset( ... ) )) to
// give it one, as TeX's \stackrel does. stackrel is \stackrel's classic plain-TeX name for
// overset: A \stackrel{\mathrm{def}}{=} B
Expr overset( Expr label, Expr base );
Expr underset( Expr label, Expr base );
Expr stackrel( Expr label, Expr base );

// A large operator (∑ ∏ ∫ ...) and its limits, either of which may be empty. In display style
// sums and products put them above and below, integrals at the side.
Expr large_operator(
	char32_t op, Expr lower = {}, Expr upper = {}, std::optional<LimitPlacement> placement = std::nullopt );
Expr sum( Expr lower = {}, Expr upper = {} );
Expr product( Expr lower = {}, Expr upper = {} );
Expr integral( Expr lower = {}, Expr upper = {} );

// A named operator (\operatorname), set upright with operator spacing, e.g. a custom
// "argmax". `moving_limits` places `lower`/`upper` over and under in display style, at the
// side in text style, like a large operator (\lim, \max, ...); otherwise they always sit at
// the side, like an ordinary subscript/superscript (\sin, \log_2, ...).
Expr named_operator( std::string name, Expr lower = {}, Expr upper = {}, bool moving_limits = false );

// The standard operators with moving limits: lim, limsup and liminf take the variable and
// where it tends to (e.g. lim( row( { ord( "x" ), rel( "→" ), ord( "0" ) } ) )); max,
// min, sup, inf and gcd normally take the set or variable a value ranges over.
Expr lim( Expr lower = {} );
Expr limsup( Expr lower = {} );
Expr liminf( Expr lower = {} );
Expr max( Expr lower = {} );
Expr min( Expr lower = {} );
Expr sup( Expr lower = {} );
Expr inf( Expr lower = {} );
Expr gcd( Expr lower = {} );

// The standard operators without limits; a subscript (such as log's base) stays at the side
Expr sin( Expr sub = {} );
Expr cos( Expr sub = {} );
Expr tan( Expr sub = {} );
Expr cot( Expr sub = {} );
Expr sec( Expr sub = {} );
Expr csc( Expr sub = {} );
Expr arcsin( Expr sub = {} );
Expr arccos( Expr sub = {} );
Expr arctan( Expr sub = {} );
Expr sinh( Expr sub = {} );
Expr cosh( Expr sub = {} );
Expr tanh( Expr sub = {} );
Expr log( Expr sub = {} );
Expr ln( Expr sub = {} );
Expr exp( Expr sub = {} );
Expr det( Expr sub = {} );
Expr dim( Expr sub = {} );
Expr deg( Expr sub = {} );
Expr arg( Expr sub = {} );

// Delimiters that stretch to fit the content; pass 0 for none
Expr fenced( char32_t open, Expr content, char32_t close );
Expr parens( Expr content );
Expr brackets( Expr content );
Expr braces( Expr content );
Expr bars( Expr content );

Expr overline( Expr content );
Expr underline( Expr content );
// A brace that stretches to fit; label it with a super/subscript on the whole thing, as TeX
// does: superscript( overbrace( content ), label )
Expr overbrace( Expr content );
Expr underbrace( Expr content );
// Any accent glyph, and some common ones
Expr accent( Expr base, char32_t accent_glyph );
Expr hat( Expr base );
Expr tilde( Expr base );
Expr bar( Expr base );
Expr dot( Expr base );
Expr vec( Expr base );

// ---- Dots ----------------------------------------------------------------------------------

// Ellipses: low dots for a list (a_1, a_2, \ldots, a_n), centered dots between operators
// (a_1 + \cdots + a_n), and the vertical/diagonal dots a matrix uses for an omitted row/diagonal
Expr ldots();
Expr cdots();
Expr vdots();
Expr ddots();

// ---- Phantom -------------------------------------------------------------------------------

// Reserves the room content would take without drawing it, so something else can line up
// with it (TeX's \phantom). hphantom keeps only the width (zero height/depth), vphantom only
// the height/depth (zero width).
Expr phantom( Expr content );
Expr hphantom( Expr content );
Expr vphantom( Expr content );

// ---- Rows --------------------------------------------------------------------------------

// One item of a row: an atom of a class (which sets the spacing around it), or extra space
struct Atom
{
	AtomClass atom = AtomClass::ORD;
	Expr expr;
	float space = 0.F;
	bool is_space = false;
};

Atom ord( Expr expr );    // variables, digits
Atom op( Expr expr );     // function names, large operators
Atom bin( Expr expr );    // + - x
Atom rel( Expr expr );    // = < >
Atom opening( Expr expr ); // ( [ {
Atom closing( Expr expr ); // ) ] }
Atom punct( Expr expr );  // , ;
Atom inner( Expr expr );  // fractions, fenced groups
// The same for plain text, so ord( "x" ) and bin( "+" ) read simply
Atom ord( std::string characters );
Atom op( std::string characters );
Atom bin( std::string characters );
Atom rel( std::string characters );
Atom opening( std::string characters );
Atom closing( std::string characters );
Atom punct( std::string characters );
Atom inner( std::string characters );
// Extra space before the next atom, in mu (Row::THIN, MEDIUM, THICK, QUAD; negative allowed)
Atom space( float mu );

// Atoms left to right, spaced as TeX does by class
Expr row( std::vector<Atom> atoms );

// ---- Grids -------------------------------------------------------------------------------

Expr matrix( std::vector<std::vector<Expr>> rows, MatrixOptions options = {} );
Expr pmatrix( std::vector<std::vector<Expr>> rows, MatrixOptions options = {} );
Expr bmatrix( std::vector<std::vector<Expr>> rows, MatrixOptions options = {} );
Expr vmatrix( std::vector<std::vector<Expr>> rows, MatrixOptions options = {} );
// Left-aligned columns after a left brace
Expr cases( std::vector<std::vector<Expr>> rows, MatrixOptions options = {} );

// ---- Tags --------------------------------------------------------------------------------

// Name a part, so the visual built from it can be found again after build() (to animate it,
// say): find_tag( built, "name" ), or a path such as find_tag( built, "fraction1", "bar" ),
// from visuals/Tags.h. The composites also name their own parts (numerator, bar, sign, ...),
// listed there, so those need no tag of yours. The tag goes on the part's own visual, so
// moving, scaling or coloring it moves, scales or colors the whole part. The same name may be
// used more than once (find_tagged returns them all, nearest first), and a part may have
// several names. Tagging an empty expression gives an empty expression.
Expr tag( std::string name, Expr content );

// ---- Style -------------------------------------------------------------------------------

// Force a style for a part, whatever it would have been (TeX's \textstyle, \displaystyle ...)
Expr with_style( MathStyle style, Expr content );

}
