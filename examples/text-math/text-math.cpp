// Text, math and code: styled and right-to-left text, laid-out formulas, marks on formulas, syntax-highlighted code, and what happens to characters no font has.
//
// Build it (see CMakeLists.txt), then
//
//     voxima --plugin build/text-math --script-dir scripts --lang en --list-scenes
//     voxima --plugin build/text-math --script-dir scripts --lang en --scene styled-text --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <code/LineHighlight.h>
#include <code/Typing.h>
#include <core/Animate.h>
#include <core/Narration.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Tween.h>
#include <core/Wait.h>
#include <math/Operators.h>
#include <visuals/CreateText.h>
#include <visuals/Layout.h>
#include <visuals/Placement.h>
#include <visuals/WrapText.h>

#include <chrono>
#include <cmath>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
void show_title( Stage &stage )
{
	// --8<-- [start:styled-text]
	StyledText text( "Hello\nالعالم" );
	text.set_font_style( 0, text.size(), FontStyle{ Intent::TITLE }.large() );
	text.set_color( 0, 5, stage.theme->palette().red );
	text.set_weight( 2, 4, Weight::BLACK );

	auto &label = stage.visuals.add( make_styled_text( *stage.theme, text ) );
	center_group( label );
	// --8<-- [end:styled-text]
	label.position = { 0.F, 3.F };
}

// Text with a script in each direction: styled runs (a color, a weight), and Arabic laid out right to left
Scene show_styled_text( Stage &stage, Script & )
{
	show_title( stage );
	co_await wait_for( stage, 2s );
}

void show_math( Stage &stage )
{
	using namespace math;

	// Formulas are described first, then built from the top in a style (display by default),
	// which hands each part its own: numerators, scripts, radicands, indices ...
	Context context( *stage.theme );

	// Lays formulas left to right from each one's own measured width, wrapping to a new,
	// lower row on its own (as well as when new_row is called) whenever one would run off
	// the right edge of the default 16x9 camera view; so nothing overlaps or overflows as
	// formulas are added, removed or resized.
	constexpr float LEFT_EDGE = -7.5F;
	constexpr float RIGHT_EDGE = 7.5F;
	constexpr float TOP_Y = 3.2F;
	constexpr float ROW_HEIGHT = 2.2F;
	constexpr float GAP = 0.6F;

	int row_index = 0;
	float x = LEFT_EDGE;
	auto place = [&]( const Expr &formula ) -> Visual &
	{
		auto visual = formula.build( context );
		const AABB box = layout_extent( *visual );
		if ( x > LEFT_EDGE && x + box.width() > RIGHT_EDGE )
		{
			++row_index;
			x = LEFT_EDGE;
		}
		const float y = TOP_Y - static_cast<float>( row_index ) * ROW_HEIGHT;
		visual->position = { x - box.min.x(), y };
		x += box.width() + GAP;
		return stage.visuals.add( std::move( visual ) );
	};
	auto new_row = [&]()
	{
		++row_index;
		x = LEFT_EDGE;
	};

	// The integral of x dx from 0 to 1 is 1/2
	place(
		row( { op( integral( text( "0" ), text( "1" ) ) ),
		       ord( "x" ),
		       space( Row::THIN ),
		       ord( upright( "d" ) ),
		       ord( "x" ),
		       rel( "=" ),
		       inner( fraction( text( "1" ), text( "2" ) ) ) } ) );

	// A root with a wide index, around a fraction
	// Parts can be found again after building, to color or animate them. The composites name
	// their own parts, so none of this needs a tag of ours.
	auto &root_formula = place( root( text( "137" ), fraction( text( "22" ), text( "7" ) ) ) );
	find_tag( root_formula, "numerator" )->color = stage.theme->palette().red;
	find_tag( root_formula, "denominator" )->color = stage.theme->palette().blue;
	find_tag( root_formula, "sign" )->color = stage.theme->palette().yellow;
	find_tag( root_formula, "index" )->color = stage.theme->palette().green;

	// A binomial coefficient
	place( binomial( text( "n" ), text( "k" ) ) );

	new_row();

	// x^2_i + 22/7 = 3.14
	auto &equation = place(
		row( { ord( tag( "power", scripts( text( "x" ), text( "2" ), text( "i" ) ) ) ),
		       bin( "+" ),
		       inner( tag( "ratio", fraction( text( "22" ), text( "7" ) ) ) ),
		       rel( "=" ),
		       ord( "3.14" ) } ) );
	// A path: the bar inside the part tagged "ratio", the superscript inside "power"
	find_tag( equation, "ratio", "bar" )->color = stage.theme->palette().yellow;
	find_tag( equation, "power", "superscript" )->color = stage.theme->palette().red;

	// A sum: in display style its limits go over and under it
	place( sum( text( "n=1" ), text( "∞" ) ) );

	// A matrix; its entries can be reached the same way, by row and column
	MatrixOptions tight{ .column_sep_em = 0.6F };
	std::vector<std::vector<Expr>> grid = { { text( "a" ), text( "b" ) }, { text( "c" ), text( "d" ) } };
	auto &matrix_formula = place( pmatrix( grid, tight ) );
	find_tag( matrix_formula, "r0c1" )->color = stage.theme->palette().green; // the entry b

	// An n x n matrix with an omitted middle: cdots across a row, vdots/ddots down and across
	std::vector<std::vector<Expr>> ellipsis_grid = {
		{ subscript( text( "a" ), text( "11" ) ), cdots(), subscript( text( "a" ), text( "1n" ) ) },
		{ vdots(), ddots(), vdots() },
		{ subscript( text( "a" ), text( "n1" ) ), cdots(), subscript( text( "a" ), text( "nn" ) ) } };
	place( bmatrix( ellipsis_grid, tight ) );

	new_row();

	// Accents
	place(
		row( { ord( hat( text( "x" ) ) ),
		       space( Row::THIN ),
		       ord( vec( text( "v" ) ) ) } ) );

	// A stretchy brace, labeled by putting a superscript on the whole thing, as TeX does
	place(
		superscript(
			overbrace( row( { ord( "a" ), bin( "+" ), ord( "b" ), bin( "+" ), ord( "c" ) } ) ),
			text( "n" ) ) );

	// A list, its middle elided with ldots (cdots elides between operators instead, above)
	place(
		row( { ord( subscript( text( "a" ), text( "1" ) ) ),
		       punct( "," ),
		       ord( subscript( text( "a" ), text( "2" ) ) ),
		       punct( "," ),
		       ord( ldots() ),
		       punct( "," ),
		       ord( subscript( text( "a" ), text( "n" ) ) ) } ) );

	// stackrel: a label stacked over a relation, always, whatever the style
	place( row( { ord( "A" ), rel( stackrel( upright( "def" ), text( "=" ) ) ), ord( "B" ) } ) );

	// phantom: the same row, then again with "bb" reserved but not drawn - same width, gap left blank
	place(
		row( { ord( "a" ),
		       bin( "+" ),
		       inner( row( { ord( "b" ), ord( "b" ) } ) ),
		       bin( "+" ),
		       ord( "c" ),
		       space( Row::QUAD ),
		       ord( "a" ),
		       bin( "+" ),
		       inner( phantom( row( { ord( "b" ), ord( "b" ) } ) ) ),
		       bin( "+" ),
		       ord( "c" ) } ) );

	// Named operators: lim moves its limit under itself in display style, sin never does
	place(
		row( { op( lim( row( { ord( "x" ), rel( "→" ), ord( "0" ) } ) ) ),
		       space( Row::THIN ),
		       inner( fraction( row( { op( sin() ), space( Row::THIN ), ord( "x" ) } ), text( "x" ) ) ),
		       rel( "=" ),
		       ord( "1" ) } ) );

	// The other math alphabets: blackboard bold, calligraphic, Fraktur, sans and mono
	place(
		row( { ord( "x" ),
		       rel( "∈" ),
		       ord( double_struck( "R" ) ),
		       punct( "," ),
		       ord( script( "A" ) ),
		       punct( "," ),
		       ord( fraktur( "g" ) ),
		       punct( "," ),
		       ord( sans( "v" ) ),
		       punct( "," ),
		       ord( mono( "t" ) ) } ) );
}

// The basics of the guide: a title, a paragraph, font styles, a formula built from small functions, and a code listing
Scene show_basics( Stage &stage, Script &script )
{
	Theme &theme = *stage.theme;
	const std::string lang = script.master_language();
	const std::string long_text = "Text is laid out with real shaping and bidirectional reordering, and wraps to a width at the spaces between words.";

	// --8<-- [start:title]
	auto &title = stage.visuals.add( make_text( theme, "Hello, Voxima", FontStyle{ Intent::TITLE }.large(), "en" ) );
	move_to( title, { 0.F, 3.F }, Anchor::TOP );
	// --8<-- [end:title]

	// --8<-- [start:paragraph]
	auto &para = stage.visuals.add( make_paragraph( theme, long_text, FontStyle{ Intent::PROSE }, 10.F, lang,
		TextLayout{ .align = TextAlign::CENTER } ) );
	// --8<-- [end:paragraph]
	move_to( para, { 0.F, 2.F }, Anchor::TOP );

	// --8<-- [start:font-styles]
	const FontStyle for_a_title{ Intent::TITLE };                       // what it is for
	const FontStyle chained = FontStyle{ Intent::PROSE }.large().bold();  // chained changes
	const FontStyle small_code = FontStyle{ Intent::CODE }.small();
	// --8<-- [end:font-styles]
	auto &styled = stage.visuals.add( make_text( theme, "Chained styles", chained, lang ) );
	move_to( styled, { -5.F, -0.5F }, Anchor::LEFT );
	auto &small = stage.visuals.add( make_text( theme, "small code", small_code, lang ) );
	move_to( small, { 2.F, -0.5F }, Anchor::LEFT );
	( void ) for_a_title;

	// --8<-- [start:formula]
	using namespace math;
	using namespace math::literals;

	Context context( theme );          // the theme and the body size; large by default

	auto formula = eq( "a"_m * "a"_m + "b"_m * "b"_m, "c"_m * "c"_m );     // aa + bb = cc
	auto &shown = stage.visuals.add( formula.build( context ) );
	// --8<-- [end:formula]
	move_to( shown, { -4.F, -2.F }, Anchor::CENTER );

	// --8<-- [start:quadratic]
	auto quadratic = row( {
		ord( "x" ), rel( "=" ),
		ord( fraction(
			row( { ord( "−b" ), bin( sym::pm ), ord( radical( row( { ord( superscript( text( "b" ), text( "2" ) ) ),
			                                                          bin( "−" ), ord( "4ac" ) } ) ) ) } ),
			text( "2a" ) ) ) } );
	// --8<-- [end:quadratic]
	auto &solution = stage.visuals.add( quadratic.build( context ) );
	move_to( solution, { 3.F, -2.5F }, Anchor::CENTER );

	// --8<-- [start:tag]
	auto halved = eq( "x"_m, fraction( tag( "half", text( "a" ) ), text( "2" ) ) );
	auto &tagged = stage.visuals.add( halved.build( context ) );
	Visual *half = find_tag( tagged, "half" );
	// --8<-- [end:tag]
	move_to( tagged, { 0.F, -3.8F }, Anchor::CENTER );
	half->color = theme.palette().red;

	co_await wait_for( stage, 3s );
}

// A page of laid-out formulas (see show_math)
Scene show_formulas( Stage &stage, Script & )
{
	show_math( stage );
	co_await wait_for( stage, 3s );
}

// Marking parts of a formula: a term cancelled, a result boxed, a part in a palette color, words between the math
Scene show_math_marks( Stage &stage, Script & )
{
	using namespace math;
	Context context( *stage.theme );
	const Palette &palette = stage.theme->palette();

	auto place = [&]( const Expr &formula, float y )
	{
		auto visual = formula.build( context );
		const AABB box = layout_extent( *visual );
		visual->position = { -( box.min.x() + box.max.x() ) / 2.F, y };
		stage.visuals.add( std::move( visual ) );
	};

	place( row( { ord( "x" ), bin( "+" ), ord( cancel( text( "3y" ) ) ), bin( "−" ), ord( cancel( text( "3y" ) ) ), rel( "=" ), ord( "x" ) } ), 2.5F );
	place( row( { ord( text( "E" ) ), rel( "=" ), ord( boxed( row( { ord( "m" ), ord( superscript( text( "c" ), text( "2" ) ) ) } ) ) ) } ), 0.8F );
	place( row( { ord( colored( palette.red, text( "a" ) ) ), bin( "+" ), ord( colored( palette.blue, text( "b" ) ) ), rel( sym::leq ), ord( primed( text( "f" ), 2 ) ) } ), -0.8F );
	place( row( { ord( text( "x" ) ), rel( sym::in ), ord( text( "S" ) ), space( Row::QUAD ), ord( words( "for all" ) ), space( Row::QUAD ), ord( text( sym::alpha ) ), rel( sym::geq ), ord( "0" ) } ), -2.5F );

	co_await wait_for( stage, 2s );

	// Written with operators and named symbols
	stage.visuals.clear();
	{
		using namespace math::literals;
		place( eq( superscript( "a"_m, "2"_m ) + superscript( "b"_m, "2"_m ), superscript( "c"_m, "2"_m ) ), 2.4F );
		place( row( { ord( "x"_m ), rel( sym::in ), ord( "S"_m ), space( Row::QUAD ), ord( -"a"_m / ( "b"_m + "c"_m ) ), rel( sym::leq ), ord( text( sym::alpha ) * text( sym::beta ) ) } ), 0.6F );
		place( row( { ord( text( sym::forall ) ), ord( "x"_m ), rel( sym::Rightarrow ), ord( "f"_m * "x"_m ), rel( sym::neq ), ord( text( sym::infty ) ) } ), -1.4F );
		place( row( { ord( upright( sym::Sigma ) ), ord( "a"_m ), bin( sym::pm ), ord( "b"_m ), bin( sym::cdot ), ord( "c"_m ), rel( sym::approx ), ord( text( sym::pi ) ) } ), -3.2F );
	}
	co_await wait_for( stage, 2s );

	// Lines lined up on the equals sign, numbered
	stage.visuals.clear();
	place( aligned( { { text( "f(x)" ), row( { rel( "=" ), ord( "(x + 1)(x − 1)" ) } ) },
					  { {}, row( { rel( "=" ), ord( superscript( text( "x" ), text( "2" ) ) ), bin( "−" ), ord( "1" ) } ) },
					  { row( { ord( primed( text( "f" ) ) ), ord( "(x)" ) } ), row( { rel( "=" ), ord( "2x" ) } ) } },
					{ .numbered = true } ),
		0.F );
	co_await wait_for( stage, 2s );

	// Wide accents, a several-line limit, a continued fraction and delimiters of chosen sizes
	stage.visuals.clear();
	place( row( { ord( widehat( text( "ABC" ) ) ), bin( "+" ), ord( widetilde( text( "xyz" ) ) ), bin( "+" ), ord( overrightarrow( text( "PQ" ) ) ) } ), 2.6F );
	place( row( { ord( sum( substack( { text( "0 < i < n" ), text( "i ≠ j" ) } ), text( "n" ) ) ), ord( text( "a" ) ) } ), 0.4F );
	place( row( { opening( big( U'(', 1 ) ), ord( "a" ), closing( big( U')', 1 ) ), space( Row::QUAD ), opening( big( U'[', 3 ) ), ord( "b" ), closing( big( U']', 3 ) ) } ), -3.2F );
	co_await wait_for( stage, 2s );

	stage.visuals.clear();
	place( row( { ord( "φ" ), rel( "=" ), ord( "1" ), bin( "+" ), ord( continued_fraction( { text( "1" ), text( "1" ), text( "1" ), text( "x" ) } ) ) } ), 1.0F );
	co_await wait_for( stage, 2s );
}

// Syntax-highlighted source code, in the theme's code font and colors, typed, and then some lines pointed out
Scene show_code( Stage &stage, Script & )
{
	// --8<-- [start:listing]
	auto listing = make_code( *stage.theme, code::Language::CPP, R"cpp(

// The sum of the squares, the long way
template<typename T>
T sum_of_squares( const std::vector<T> &values )
{
	constexpr int START = 0;
	T total = START;
	for ( const T &v: values )
	{
		total += v * v; // no overflow check
	}
	return total;
}
)cpp" );
	center_group( *listing );
	auto &shown = stage.visuals.add( std::move( listing ) );
	// --8<-- [end:listing]

	// Typed, like a person would
	// --8<-- [start:typing]
	Typewriter typer( shown );
	co_await typer.type( stage );
	co_await wait_for( stage, 1s );

	// Then pointed out, as narration would: a line, a range of lines, and two separate places, and away again
	LineHighlight pointer( shown, *stage.theme );
	co_await pointer.to( stage, Lines( 5 ), 0.4s );                        // the signature
	co_await wait_for( stage, 1.5s );
	co_await pointer.to( stage, Lines( 9, 12 ), 0.5s );                    // the loop
	co_await wait_for( stage, 1.5s );
	co_await pointer.to( stage, Lines{ { 7, 8 }, { 13, 13 } }, 0.5s );     // where it starts, and what it gives back
	co_await wait_for( stage, 1.5s );
	co_await pointer.to( stage, Lines(), 0.4s );
	// --8<-- [end:typing]
	co_await wait_for( stage, 1s );
}

// Text that cannot be drawn: a script value that is missing turns into U+FFFD, and no font has the second
// character. Both show as magenta boxes with their code in them, and are listed when the render finishes.
Scene show_missing_glyphs( Stage &stage, Script &script )
{
	const std::string lang = script.master_language();
	const std::string missing_value{ script.fetch( lang, "no such key" ).as_string( "\uFFFD" ) };

	const std::vector<std::string> lines = {
		"A script value that is missing: " + missing_value,
		"A character no font has: \U0010FFFD",
		"and one that does exist: \u00E9" };

	float y = 1.5F;
	for ( const auto &line: lines )
	{
		auto &text = stage.visuals.add( make_text( *stage.theme, line, FontStyle{ Intent::PROSE }.large(), lang ) );
		center_group( text );
		text.position = { 0.F, y };
		y -= 1.2F;
	}

	co_await wait_for( stage, 2s );
}
}

REGISTER_SCENE( "styled-text", "Styled text, and Arabic laid out right to left", show_styled_text );
REGISTER_SCENE( "text-basics", "A title, a paragraph, font styles, and formulas built from small functions", show_basics );
REGISTER_SCENE( "formulas", "A page of laid-out math formulas", show_formulas );
REGISTER_SCENE( "math-marks", "Cancelled and boxed parts, colors, primes, and words between math", show_math_marks );
REGISTER_SCENE( "code", "Syntax-highlighted C++, typed, with lines pointed out", show_code );
REGISTER_SCENE( "missing-glyphs", "Characters that cannot be drawn, shown as magenta boxes (and reported)", show_missing_glyphs );
