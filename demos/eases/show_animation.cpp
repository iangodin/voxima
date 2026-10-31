// The demo's scenes. With no --scene the "movie" scene renders (the morph scenes, then the karaoke); any one can be
// rendered on its own, or several in order:
//
//     voxima --plugin build/Release/eases ... --scene morph-text
//     voxima --plugin build/Release/eases ... --scene morph-shapes --scene karaoke
//     voxima --plugin build/Release/eases --list-scenes
//
// Not part of the movie, so they are only rendered when asked for:
//   missing-glyphs  characters that cannot be drawn, as magenta boxes (it warns at the end of the render, so
//                   it would make every full render do so)
//   eases           the ease functions racing across the screen, with narration
//   formulas        a page of laid-out math
//
// See doc/scenes.md.



#include <core/Animate.h>
#include <core/Coroutine.h>
#include <core/Director.h>
#include <core/Plugin.h>
#include <core/EaseBack.h>
#include <core/EaseCubic.h>
#include <core/EaseElastic.h>
#include <core/EaseLinear.h>
#include <core/Scene.h>
#include <core/Screenplay.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>

#include <math/Expr.h>
#include <math/Operators.h>
#include <math/Symbols.h>
#include <visuals/Tags.h>
#include <visuals/Circle.h>
#include <plot/BarChart.h>
#include <plot/DrawAxes.h>
#include <plot/Legend.h>
#include <plot/NumberLine.h>
#include <plot/Scatter.h>
#include <visuals/Brace.h>
#include <visuals/Callout.h>
#include <visuals/Connector.h>
#include <visuals/Diagram.h>
#include <visuals/Geometry.h>
#include <visuals/WrapText.h>
#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
#include <visuals/StyledText.h>
#include <visuals/CreateMathLargeOperation.h>
#include <visuals/Fraction.h>
#include <visuals/Layout.h>
#include <visuals/Radical.h>
#include <visuals/Accent.h>
#include <visuals/Bars.h>
#include <visuals/Delimited.h>
#include <visuals/LargeOperator.h>
#include <visuals/Matrix.h>
#include <visuals/Row.h>
#include <visuals/Scripts.h>
#include <fonts/MathFont.h>
#include <fonts/MathStyle.h>

#include <core/Narration.h>
#include <core/WordEffects.h>
#include <utils/Markers.h>
#include <utils/Words.h>
#include <core/SceneRegistry.h>
#include <core/MorphAnim.h>
#include <core/CameraMove.h>
#include <core/Tween.h>
#include <code/CreateCode.h>
#include <code/LineHighlight.h>
#include <code/Typing.h>
#include <plot/Axes.h>
#include <visuals/CreateList.h>
#include <visuals/CreateTable.h>
#include <visuals/Panel.h>
#include <visuals/Placement.h>
#include <plot/Graph.h>
#include <visuals/ArrowTip.h>
#include <visuals/Image.h>
#include <visuals/Line.h>
#include <visuals/Rectangle.h>

#include <chrono>
#include <numbers>
#include <cmath>
#include <filesystem>
#include <iostream>

// Times in the scenes are written in seconds (0.5s, 1.7s), so they last as long at any --fps
using namespace std::chrono_literals;

void show_title( Stage &stage )
{
	StyledText text( "Hello\nالعالم" );
	text.set_font_style( 0, text.size(), FontStyle{ Intent::TITLE }.large() );
	text.set_color( 0, 5, stage.theme->palette().red );
	text.set_weight( 2, 4, Weight::BLACK );

	auto &label = stage.visuals.add( make_styled_text( *stage.theme, text ) );
	center_group( label );
	label.position = { 0.F, 3.F };
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

template<typename Ease>
Scene show_ease(
	Stage &stage, const std::string &name, const std::string &lang, Ease &&ease, float y, std::chrono::duration<double> duration, const Color &color )
{
	float x1 = -4.F;
	float x2 = 7.F;
	auto &circle = stage.visuals.add( std::make_unique<Circle>() );
	circle.radius = 0.1F;
	circle.color = color;
	circle.position = { x1, y };

	auto &label = stage.visuals.add( make_text( *stage.theme, name, FontStyle{ Intent::LABEL }, lang ) );
	label.position = { -6.F, y };

	co_await tween(
		stage, duration, ease, [&]( float t ) { circle.position = { x1 + ( x2 - x1 ) * t, circle.position.y() }; } );
}

std::unique_ptr<Circle> demo_circle( const Stage &stage, Point at )
{
	const Palette &palette = stage.theme->palette();
	auto c = std::make_unique<Circle>();
	c->radius = 0.8F;
	c->position = at;
	c->color = palette.red;
	return c;
}

std::unique_ptr<Rectangle> demo_square( const Stage &stage, Point at )
{
	auto r = std::make_unique<Rectangle>();
	r->half_width = r->half_height = 0.7F;
	r->position = at;
	r->color = stage.theme->palette().blue;
	return r;
}

std::unique_ptr<Group> demo_letter( Stage &stage, const char *s, const Color &color, Point at )
{
	auto text = make_text( *stage.theme, s, FontStyle{ Intent::PROSE }.huge() );
	center_group( *text );
	text->position = at;
	text->color = color;
	return text;
}

// A plucked note, made rather than read from a file: a sine that dies away over `seconds`
AudioBuffer pluck( float frequency, float seconds = 2.F )
{
	constexpr size_t rate = 48000;
	AudioBuffer note( static_cast<size_t>( seconds * rate ), rate, 1 );
	for ( size_t f = 0; f < note.frames(); ++f )
	{
		const float t = static_cast<float>( f ) / rate;
		const float attack = std::min( 1.F, t / 0.005F ); // no click at the start
		note.frame( f )[0] = attack * std::exp( -3.F * t / seconds * 2.F ) * std::sin( 2.F * 3.14159265F * frequency * t );
	}
	return note;
}

// A shape that turns into other shapes and letters, one after the other: circle, square, then glyphs
// (with holes that open and close) and back to the circle. Each change sounds a two-note chord (sound effects
// play together, and ring on into the next).
Scene show_morph( Stage &stage, Script & )
{
	const Point home = { 0.F, 2.5F };
	auto circle = [&]() { return demo_circle( stage, home ); };
	auto square = [&]() { return demo_square( stage, home ); };
	auto letter = [&]( const char *s, const Color &color ) { return demo_letter( stage, s, color, home ); };

	Visual *shown = &stage.visuals.add( circle() );
	co_await wait_for( stage, 0.5s );

	std::vector<std::unique_ptr<Visual>> targets;
	targets.push_back( square() );
	targets.push_back( letter( "a", stage.theme->palette().yellow ) );
	targets.push_back( letter( "e", stage.theme->palette().green ) );
	targets.push_back( letter( "8", Color{ 1.F, 1.F, 1.F, 1.F } ) );
	targets.push_back( letter( "x", stage.theme->palette().blue ) );
	targets.push_back( circle() );

	const float notes[] = { 261.6F, 293.7F, 329.6F, 392.0F, 440.0F, 523.3F }; // C D E G A C
	size_t note = 0;
	for ( auto &target: targets )
	{
		Visual *next = target.get();
		const float root = notes[note++ % std::size( notes )];
		stage.audio.play( pluck( root ), { .volume = -14.0 } );
		stage.audio.play( pluck( root * 1.5F ), { .volume = -18.0 } ); // and a fifth above
		co_await morph( stage, *shown, std::move( target ), 1.5s, ease_cubic_in_out );
		shown = next;
		co_await wait_for( stage, 0.5s );
	}
}

// A formula that is rewritten step by step. Parts that stay the same (the a, the b, the plus signs) only
// slide into place, the rest morph or fade: leaves are matched by their tags, then by identical glyphs.
Scene show_formula_morph( Stage &stage, Script & )
{
	using namespace math;
	Context context( *stage.theme );

	auto build = [&]( const Expr &formula )
	{
		auto visual = formula.build( context );
		const AABB box = layout_extent( *visual );
		visual->position = { -( box.min.x() + box.max.x() ) / 2.F, -2.5F };
		return visual;
	};

	// Tagged parts are followed from one formula to the next wherever they end up; everything else is matched
	// by identical glyphs, and what has no counterpart fades (the parentheses, the new terms)
	const Expr a = tag( "a", text( "a" ) );
	const Expr b = tag( "b", text( "b" ) );
	const Expr expanded = row(
		{ ord( superscript( a, text( "2" ) ) ),
		  bin( "+" ),
		  ord( text( "2ab" ) ),
		  bin( "+" ),
		  ord( superscript( b, text( "2" ) ) ) } );

	Visual *shown = &stage.visuals.add( build( superscript( parens( row( { ord( a ), bin( "+" ), ord( b ) } ) ), text( "2" ) ) ) );
	co_await wait_for( stage, 0.5s );

	std::vector<std::unique_ptr<Visual>> steps;
	steps.push_back( build( expanded ) );
	steps.push_back( build( radical( expanded ) ) );

	for ( auto &step: steps )
	{
		Visual *next = step.get();
		co_await morph( stage, *shown, std::move( step ), 1.7s, ease_cubic_in_out );
		shown = next;
		co_await wait_for( stage, 0.5s );
	}
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

// A sentence that is edited: words that stay put only slide into place, words that go away fade out, new
// ones fade in, and a word that is replaced turns into its replacement letter by letter.
Scene show_text_morph( Stage &stage, Script & )
{
	auto line = [&]( const char *sentence )
	{
		auto text = make_text( *stage.theme, sentence, FontStyle{ Intent::PROSE }.large() );
		center_group( *text );
		text->position = { 0.F, -1.2F };
		return text;
	};

	Visual *shown = &stage.visuals.add( line( "The big cat sat down" ) );
	co_await wait_for( stage, 0.5s );

	std::vector<std::unique_ptr<Visual>> sentences;
	sentences.push_back( line( "The cat sat down" ) );        // "big" fades away
	sentences.push_back( line( "The cat sat on the mat" ) );  // "on the mat" fades in
	sentences.push_back( line( "The dog ran off" ) );         // words replaced, "The" stays

	for ( auto &sentence: sentences )
	{
		Visual *next = sentence.get();
		co_await morph( stage, *shown, std::move( sentence ), 1.7s, ease_cubic_in_out );
		shown = next;
		co_await wait_for( stage, 0.5s );
	}
}

// Strokes, arrows, letters that morph one after another, and pictures: a line inflates into a disc, which
// becomes an arrow that then turns; the arrow becomes a word whose letters change in a ripple; the word
// becomes a picture, which moves and fades into a disc.
Scene show_more_morphs( Stage &stage, Script & )
{
	const Point spot = { 0.F, 1.F };

	auto line = std::make_unique<Line>();
	line->start = { -2.F, 0.F };
	line->end = { 2.F, 0.F };
	line->stroke_thickness = 0.12F;
	line->stroke_color = stage.theme->palette().yellow;
	line->position = spot;

	auto arrow = [&]( float degrees )
	{
		auto group = std::make_unique<Group>();
		auto shaft = std::make_unique<Line>();
		shaft->start = { -1.5F, 0.F };
		shaft->end = { 1.2F, 0.F };
		shaft->stroke_thickness = 0.12F;
		shaft->stroke_color = stage.theme->palette().green;
		auto tip = std::make_unique<ArrowTip>();
		tip->start = { -1.5F, 0.F };
		tip->end = { 1.6F, 0.F };
		tip->size = 0.3F;
		tip->color = stage.theme->palette().green;
		group->add( std::move( shaft ) );
		group->add( std::move( tip ) );
		group->position = spot;
		group->rotation = degrees;
		return group;
	};
	auto word = [&]( const char *s )
	{
		auto text = make_text( *stage.theme, s, FontStyle{ Intent::PROSE }.huge() );
		center_group( *text );
		text->position = spot;
		return text;
	};
	auto picture = [&]( Point at, float scale )
	{
		auto image = std::make_unique<Image>();
		image->path = "assets/images/testcard.png";
		image->half_width = image->half_height = 1.2F;
		image->position = at;
		image->scale_x = image->scale_y = scale;
		return image;
	};

	struct Step
	{
		std::unique_ptr<Visual> to;
		std::chrono::duration<double> duration;
		MorphOptions options;
	};
	std::vector<Step> steps;
	steps.push_back( { demo_circle( stage, spot ), 1.7s, {} } );  // the line inflates into a disc
	steps.push_back( { arrow( 0.F ), 1.7s, {} } );         // the disc becomes an arrow, and
	steps.push_back( { arrow( 60.F ), 1.7s, {} } );        // the arrow turns
	MorphOptions ripple;
	ripple.stagger = 0.6F;
	ripple.part_ease = []( float t ) { return t * t * ( 3.F - 2.F * t ); };
	steps.push_back( { word( "smooth" ), 1.7s, {} } );     // the arrow becomes a word
	steps.push_back( { word( "morph" ), 2.5s, ripple } );  // whose letters change one after another
	steps.push_back( { picture( spot, 1.F ), 1.7s, {} } ); // the word cross-fades into a picture
	steps.push_back( { picture( { 3.F, spot.y() }, 0.6F ), 1.7s, {} } ); // which moves and shrinks
	steps.push_back( { demo_circle( stage, { 3.F, spot.y() } ), 1.7s, {} } );   // and fades into a disc

	Visual *shown = &stage.visuals.add( std::move( line ) );
	co_await wait_for( stage, 0.5s );
	for ( auto &step: steps )
	{
		Visual *next = step.to.get();
		co_await morph( stage, *shown, std::move( step.to ), step.duration, ease_cubic_in_out, step.options );
		shown = next;
		co_await wait_for( stage, 0.5s );
	}
}

// A shape that changes on the phrases being said: a circle turns into a square between the markers {a} and {b}
// in the script, into an a from {b} to {c}, and back into a circle from {c} to {d}. The markers are in the text of
// every language, so this follows the narration whichever language it is in (word numbers would not).
Scene follow_speech( Stage &stage, Narration &narration )
{
	const Point spot = { 5.5F, 2.5F };
	auto smooth = []( float t ) { return t * t * ( 3.F - 2.F * t ); };

	Visual *shown = &stage.visuals.add( demo_circle( stage, spot ) );

	auto square = demo_square( stage, spot );
	Visual *next = square.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( square ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_markers( "a", "b", [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
	shown = next;

	auto letter = demo_letter( stage, "a", stage.theme->palette().yellow, spot );
	next = letter.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( letter ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_markers( "b", "c", [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
	shown = next;

	auto circle = demo_circle( stage, spot );
	next = circle.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( circle ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_markers( "c", "d", [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
}

// Narrated text with an effect on every word as it is said. The first line is dim until its words are spoken:
// each lights up yellow, pops, and settles to the text color. The second line is invisible until its words are spoken:
// each fades in, flashes yellow, and is underlined as it is said. Beside them a shape changes on the words.
Scene show_karaoke( Stage &stage, Script &script )
{
	using namespace word_effects;

	const std::string lang = script.master_language();
	// The colors of the palette, so the words show on a light background as on a dark one: not yet said (muted), said
	// (the palette's yellow: a dark gold on a light palette), and said before (the text color itself: what the effect
	// blends to has to be a color, not "white", or a word passes through a pale gray on its way to dark text)
	const Palette &palette = stage.theme->palette();
	const Color dim = palette.muted;
	const Color white = stage.foreground;
	const Color yellow = palette.yellow;

	Narration narration;
	narration.setup( "narrator", script, stage.audio );
	narration.speech( "intro" );

	// One text per narrated line, shown as it is written: word N spoken is word N of the line, from the first
	// word of that line on.
	std::vector<Coroutine> running;
	running.push_back( narration.play() );
	running.push_back( follow_speech( stage, narration ) );

	size_t first_word = 0;
	size_t line_number = 0;
	const auto lines = script.fetch( lang, "intro" ).as_multiline();
	for ( const auto &line: lines )
	{
		const std::string clean = strip_markers( line ); // the markers are cue points, not text
		StyledText styled( clean );
		styled.set_font_style( 0, styled.size(), FontStyle{ Intent::PROSE }.large() );
		styled.set_language( 0, styled.size(), lang );

		auto &text = stage.visuals.add( make_styled_text( *stage.theme, styled ) );
		center_group( text );
		text.position = { 0.F, 1.F - 0.9F * static_cast<float>( line_number ) };

		const WordEffect effect = line_number % 2 == 0
									  ? all( { color( dim, yellow, white ), pop( 1.15F ) } )
									  : all( { reveal(), color( white, yellow, white ), underline( yellow ) } );
		running.push_back( narration.animate_words( text, effect, first_word ) );

		first_word += split_words( clean ).size();
		++line_number;
	}

	co_await choreograph_all( std::move( running ) );
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

// The ease functions side by side, as circles racing across the screen while the narrator speaks
Scene show_eases( Stage &stage, Script &script )
{
	const auto duration = 1s;

	const std::string not_found{ "\uFFFD" };
	const std::string lang = script.master_language();
	auto label = [&]( const char *key ) { return std::string( script.fetch( lang, "labels", key ).as_string( not_found ) ); };

	Narration narration;
	narration.setup( "narrator", script, stage.audio );
	narration.speech( "intro" );

	// Each curve in a color of the palette, in turn: the lanes can be told apart
	const auto series = stage.theme->palette().series();
	auto accent = [&series]( size_t i ) { return series[i % series.size()]; };

	co_await choreograph(
		show_ease( stage, label( "ease_cubic_in" ), lang, ease_cubic_in, 3.F, duration, accent( 0 ) ),
		show_ease( stage, label( "ease_cubic_out" ), lang, ease_cubic_out, 2.5F, duration, accent( 1 ) ),
		show_ease( stage, label( "ease_cubic_in_out" ), lang, ease_cubic_in_out, 2.0F, duration, accent( 2 ) ),
		show_ease( stage, label( "ease_elastic_in" ), lang, ease_elastic_in, 1.5F, duration, accent( 3 ) ),
		show_ease( stage, label( "ease_elastic_out" ), lang, ease_elastic_out, 1.0F, duration, accent( 4 ) ),
		show_ease( stage, label( "ease_elastic_in_out" ), lang, ease_elastic_in_out, 0.5F, duration, accent( 5 ) ),
		show_ease( stage, label( "ease_back_in" ), lang, ease_back_in, 0.F, duration, accent( 6 ) ),
		show_ease( stage, label( "ease_back_out" ), lang, ease_back_out, -0.5F, duration, accent( 7 ) ),
		show_ease( stage, label( "ease_back_in_out" ), lang, ease_back_in_out, -1.0F, duration, accent( 8 ) ),
		show_ease( stage, label( "ease_linear" ), lang, ease_linear, -1.5F, duration, accent( 9 ) ),
		narration.play() );
}

// A page of laid-out formulas (see show_math)
Scene show_formulas( Stage &stage, Script & )
{
	show_math( stage );
	co_await wait_for( stage, 3s );
}

// Syntax-highlighted source code, in the theme's code font and colors, typed, and then some lines pointed out
Scene show_code( Stage &stage, Script & )
{
	auto listing = make_code( *stage.theme, code::Language::CPP, R"cpp(
#include <vector>

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

	// Typed, like a person would
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
	co_await wait_for( stage, 1s );
}

// Axes with a grid, and two graphs drawn on them: a sine wave, then 1/x (in two pieces, either side of 0)
Scene show_plot_with( Stage &stage, AxisPlacement placement )
{
	AxesOptions options;
	options.placement = placement;
	options.x = { -6.0, 6.0 };
	options.y = { -2.5, 2.5 };
	options.width = 12.F;
	options.height = 5.F;
	options.grid = true;
	auto &axes = stage.visuals.add( make_axes( *stage.theme, options ) );
	co_await draw_axes( stage, axes, 2.5s );
	co_await wait_for( stage, 0.3s );

	auto &wave = axes.add_plot( plot( axes, []( double x ) { return std::sin( x ); }, { .name = "sin x" } ) );
	co_await draw( stage, wave, 2s );

	GraphOptions orange;
	orange.color = stage.theme->palette().orange;
	orange.name = "1 / x";
	auto &inverse = axes.add_plot( plot( axes, []( double x ) { return 1.0 / x; }, orange ) );
	co_await draw( stage, inverse, 2s );

	// A key to the two curves
	auto &key = add_legend( axes, make_legend( *stage.theme, { legend_entry( wave ), legend_entry( inverse ) } ), Anchor::TOP_RIGHT );
	co_await fade_in( stage, key, 0.6s );
	co_await wait_for( stage, 2s );
}

// A sine wave, and the camera zooming in on its peaks and panning along, then back out
Scene show_camera( Stage &stage, Script & )
{
	AxesOptions options;
	options.x = { -6.0, 6.0 };
	options.y = { -2.0, 2.0 };
	options.width = 12.F;
	options.height = 4.F;
	options.grid = true;
	options.placement = AxisPlacement::EDGE;
	auto &axes = stage.visuals.add( make_axes( *stage.theme, options ) );
	axes.add_plot( plot( axes, []( double x ) { return std::sin( x ); } ) );
	co_await wait_for( stage, 1s );

	const double pi = std::numbers::pi;
	// In on the first peak, (pi / 2, 1): a look at 3 times the size, moving on to the next peak, and out again
	const Point peak = axes.at( pi / 2, 1.0 );
	co_await camera_to( stage, { peak, 3.F }, 1.5s );
	co_await wait_for( stage, 1s );
	co_await camera_to( stage, { axes.at( 3 * pi / 2, -1.0 ), 3.F }, 2s ); // then the trough after it
	co_await wait_for( stage, 1s );
	co_await camera_to( stage, { { 0.F, 0.F }, 1.F }, 1.5s );
	co_await wait_for( stage, 1s );
}

// ... with the axes crossing in the middle
Scene show_plot( Stage &stage, Script & )
{
	co_await show_plot_with( stage, AxisPlacement::ORIGIN );
}

// ... and along the left and bottom edges
Scene show_plot_edge( Stage &stage, Script & )
{
	co_await show_plot_with( stage, AxisPlacement::EDGE );
}

// Things placed against each other, not on coordinates: a list and a table in panels, and a row of labeled shapes
Scene show_layout( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();

	// A title, at the top
	auto &title = stage.visuals.add( make_text( theme, "Layout", FontStyle{ Intent::TITLE }.large() ) );
	move_to( title, { 0.F, 4.1F }, Anchor::TOP );

	// A list in a panel, and a table in another, side by side
	// (a panel with a shadow has an opaque fill: the background lifted a little toward the text color)
	const Color lifted{ palette.background.r * 0.88F + palette.text.r * 0.12F, palette.background.g * 0.88F + palette.text.g * 0.12F, palette.background.b * 0.88F + palette.text.b * 0.12F, 1.F };
	auto list_panel = make_panel(
		make_list( theme, { "Rows and columns", "Grids of any size", "Panels that fit", "Numbered lists too" } ),
		{ .fill = lifted, .shadow = ShadowOptions{} } );
	auto table_panel = make_panel(
		make_table( theme,
		{ { "Palette", "Type", "Background" },
		  { "paper", "light", "warm white" },
		  { "one-dark", "dark", "slate" },
		  { "blueprint", "dark", "navy" } },
		{ .striped = true, .alignment = { Align::START, Align::CENTER, Align::END } } ) );
	auto &left = stage.visuals.add( std::move( list_panel ) );
	auto &right = stage.visuals.add( std::move( table_panel ) );
	move_to( left, { -3.6F, 1.2F }, Anchor::CENTER );
	next_to( right, left, Anchor::RIGHT, 0.7F );

	// A row of shapes, each with its label under it, the row centered under the panels
	Group shapes;
	auto shape_with = [&]( std::unique_ptr<Visual> shape, const std::string &name )
	{
		auto column = std::make_unique<Group>();
		column->add( std::move( shape ) );
		// The label's baseline at the same depth in every column (a text's box depends on the letters in it, so lining
		// up the boxes would not line up the words), centered under the shape
		auto &label = column->add( make_text( theme, name, FontStyle{ Intent::LABEL } ) );
		const AABB box = box_of( label );
		label.position = { -( box.min.x() + box.max.x() ) / 2.F, -1.05F };
		return column;
	};
	auto circle = std::make_unique<Circle>( 0.5F, palette.red );
	auto square = std::make_unique<Rectangle>( 0.5F, 0.5F, palette.blue );
	auto bar = std::make_unique<Rectangle>( 1.1F, 0.4F, palette.green );
	auto &row = stage.visuals.add( std::make_unique<Group>() );
	row.add( shape_with( std::move( circle ), "circle" ) );
	row.add( shape_with( std::move( square ), "square" ) );
	row.add( shape_with( std::move( bar ), "a longer bar" ) );
	arrange_row( row, { .gap = 1.2F, .align = Align::BASELINE } ); // the columns' origins level: shapes centered, labels on one line
	next_to( row, left, Anchor::BOTTOM, 0.9F );
	move_to( row, { 0.F, point_at( row, Anchor::CENTER ).y() }, Anchor::CENTER );

	co_await wait_for( stage, 3s );
}

// The animation verbs: things fade, slide and grow in, a connector follows them as they move, one is pointed out, and they leave
Scene show_verbs( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();

	auto node = [&]( Color color )
	{
		auto circle = std::make_unique<Circle>();
		circle->radius = 0.7F;
		circle->color = color;
		auto &shape = stage.visuals.add( std::move( circle ) );
		return &shape;
	};
	auto *a = node( palette.blue );
	auto *b = node( palette.green );
	auto *c = node( palette.orange );
	a->position = { -4.F, 0.F };
	b->position = { 0.F, 0.F };
	c->position = { 4.F, 0.F };
	auto &ab = stage.visuals.add( make_connector( *a, *b, { .gap = 0.1F } ) );
	auto &bc = stage.visuals.add( make_connector( *b, *c, { .gap = 0.1F } ) );
	auto tie_ab = follow( stage, ab );
	auto tie_bc = follow( stage, bc );

	auto &caption = stage.visuals.add( make_text( theme, "Fade, slide, grow, indicate", FontStyle{ Intent::TITLE } ) );
	move_to( caption, { 0.F, 3.5F }, Anchor::TOP );

	co_await choreograph( fade_in( stage, *a, 0.5s ), slide_in( stage, *b, { 0.F, -3.F }, 0.8s ), grow_in( stage, *c, 0.6s ), fade_in( stage, ab, 0.8s ), fade_in( stage, bc, 0.8s ) );
	co_await wait_for( stage, 0.3s );
	co_await indicate( stage, *b );
	co_await move_by( stage, *b, { 0.F, 2.5F }, 0.8s, ease_cubic_in_out ); // the connectors bend with it
	co_await move_by( stage, *b, { 0.F, -2.5F }, 0.8s, ease_cubic_in_out );
	co_await wait_for( stage, 0.3s );
	co_await choreograph( fade_out( stage, *a, 0.5s ), slide_out( stage, *b, { 0.F, 3.F }, 0.8s ), shrink_out( stage, *c, 0.6s ), fade_out( stage, ab, 0.5s ), fade_out( stage, bc, 0.5s ) );
	co_await wait_for( stage, 0.5s );

	// Drawn on: a shape's outline traced, and text written
	auto &ring = stage.visuals.add( std::make_unique<Circle>() );
	ring.radius = 1.2F;
	ring.color = palette.purple;
	ring.position = { -3.F, -0.5F };
	auto &words = stage.visuals.add( make_text( theme, "Draw me", FontStyle{ Intent::TITLE }.large() ) );
	move_to( words, { 2.5F, -0.5F }, Anchor::CENTER );
	co_await choreograph( draw_on( stage, ring, 1.6s ), draw_on( stage, words, 1.6s ) );
	co_await wait_for( stage, 0.5s );

	// Pointing out
	co_await circumscribe( stage, words );
	co_await flash( stage, ring );
	co_await wait_for( stage, 0.5s );

	// Staggered: a row of squares, each coming in a little after the last; then a bar along a curve, turning to face its way
	stage.visuals.clear();
	stage.updaters.clear();
	auto &row = stage.visuals.add( std::make_unique<Group>() );
	for ( int i = 0; i < 6; ++i )
	{
		auto square = std::make_unique<Rectangle>();
		square->half_width = square->half_height = 0.4F;
		square->color = palette.series()[static_cast<size_t>( i ) % palette.series().size()];
		square->position = { -4.F + 1.2F * static_cast<float>( i ), 2.2F };
		row.add( std::move( square ) );
	}
	co_await stagger( stage, children_of( row ), 0.12s, [&]( Visual &v ) { return slide_in( stage, v, { 0.F, 1.5F }, 0.5s ); } );

	auto &dart = stage.visuals.add( std::make_unique<Rectangle>() );
	dart.half_width = 0.4F;
	dart.half_height = 0.12F;
	dart.color = palette.orange;
	Path path;
	path.commands = { MoveTo{ { -4.5F, -2.F } }, CubicTo{ { 0.F, 0.F }, { -3.F, 2.F }, { 3.F, -2.5F } }, CubicTo{ { 4.5F, 0.F }, { 3.5F, 1.F }, { 4.5F, -0.5F } } };
	dart.position = { -4.5F, -2.F };
	co_await move_along( stage, dart, path, 2.5s, ease_cubic_in_out, true );
	co_await turn_about( stage, dart, { 4.5F, -0.5F }, 360.F, 1.5s, ease_cubic_in_out );
	co_await wait_for( stage, 0.5s );
}

// A diagram laid out in layers, a callout pointing into it, a wrapped list, and things spread out evenly
Scene show_diagram( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();
	const FontStyle label{ Intent::LABEL };

	auto tinted = [&]( Color accent )
	{
		return PanelOptions{ .fill = { accent.r, accent.g, accent.b, 0.25F }, .outline = accent, .outline_width = 0.04F };
	};

	auto &flow = stage.visuals.add( make_diagram() );
	flow.add_text_node( theme, "read", "Read input", label, {}, tinted( palette.blue ) );
	flow.add_text_node( theme, "parse", "Parse", label, {}, tinted( palette.green ) );
	flow.add_text_node( theme, "check", "Validate", label, {}, tinted( palette.green ) );
	flow.add_text_node( theme, "run", "Run", label, {}, tinted( palette.orange ) );
	flow.add_text_node( theme, "log", "Write a log", label, {}, tinted( palette.purple ) );
	flow.add_edge( "read", "parse" );
	flow.add_edge( "read", "check" );
	flow.add_edge( "parse", "run" );
	flow.add_edge( "check", "run" );
	flow.add_edge( "run", "log" );
	flow.layout_layers( { .layer_gap = 0.9F } );
	// Off to the left: the nodes moved (not the diagram, so the callout, in the stage's frame, finds them where they are)
	for ( const char *id: { "read", "parse", "check", "run", "log" } )
	{
		flow.node( id ).position = flow.node( id ).position + Point{ -3.5F, 0.F };
	}
	flow.update();

	auto &note = stage.visuals.add( make_callout(
		make_paragraph( theme, "Both checks must finish before anything runs", label, 3.6F ),
		flow.node( "run" ) ) );
	next_to( note.panel(), flow.node( "run" ), Anchor::RIGHT, 1.6F );
	note.update();

	auto &list = stage.visuals.add( make_list( theme, { "Each step is a node", "Arrows say what waits for what", "The layers are worked out for you" }, { .max_width = 5.2F } ) );
	move_to( list, { 4.2F, -2.6F }, Anchor::CENTER );

	// A brace along the list's left, with its label
	stage.visuals.add( make_labeled_brace_beside( theme, list, Anchor::LEFT, "3 ideas", label, 0.25F, { .color = palette.muted } ) );

	// A number line, with a mark and an interval
	auto &numbers = stage.visuals.add( make_number_line( theme, { .range = { 0.0, 10.0 }, .length = 7.F } ) );
	numbers.position = { 3.8F, 1.4F };
	numbers.add_interval( theme, 6.0, 9.0, palette.blue, { .closed_to = false } );
	numbers.add_mark( theme, 2.5, "x", palette.orange );

	// Three things of different sizes, spread evenly along the top
	auto &top = stage.visuals.add( std::make_unique<Group>() );
	for ( const char *word: { "Short", "A longer heading", "Mid" } )
	{
		top.add( make_text( theme, word, FontStyle{ Intent::TITLE } ) );
	}
	arrange_row( top, { .gap = 0.F } );
	{
		int i = 0;
		for ( const auto &child: top )
		{
			child->position = child->position + Point{ i == 0 ? -5.F : ( i == 1 ? 0.F : 5.F ), 0.F };
			++i;
		}
	}
	distribute_row( top );
	move_to( top, { top.position.x(), 4.0F }, Anchor::TOP );
	keep_inside( top, safe_area( stage.camera ) );

	co_await wait_for( stage, 2s );

	// The same kind of thing going across, built up node by node: the boxes come in one after another, then the arrows, then a
	// node is pointed out and moved, the arrows following it
	stage.visuals.clear();
	stage.updaters.clear();
	auto &pipeline = stage.visuals.add( make_diagram() );
	for ( const auto &[id, name, accent]: { std::tuple{ "in", "Input", palette.blue }, std::tuple{ "clean", "Clean", palette.green }, std::tuple{ "model", "Model", palette.orange }, std::tuple{ "out", "Report", palette.purple } } )
	{
		pipeline.add_text_node( theme, id, name, label, {}, tinted( accent ) );
	}
	pipeline.add_edge( "in", "clean" );
	pipeline.add_edge( "clean", "model" );
	pipeline.add_edge( "model", "out" );
	pipeline.layout_layers( { .direction = Direction::RIGHT, .layer_gap = 1.4F } );
	auto tie = follow( stage, pipeline );

	// (nodes hidden until their turn: the arrows are drawn over the space between)
	for ( size_t i = 0; i < pipeline.edge_count(); ++i )
	{
		pipeline.edge( i ).color.a = 0.F;
		pipeline.edge( i ).stroke_color.a = 0.F;
	}
	std::vector<Visual *> nodes;
	for ( const char *id: { "in", "clean", "model", "out" } )
	{
		nodes.push_back( &pipeline.node( id ) );
	}
	co_await stagger( stage, nodes, 0.25s, [&]( Visual &node ) { return grow_in( stage, node, 0.5s ); } );
	std::vector<Visual *> arrows;
	for ( size_t i = 0; i < pipeline.edge_count(); ++i )
	{
		arrows.push_back( &pipeline.edge( i ) );
	}
	for ( Visual *arrow: arrows )
	{
		arrow->color.a = 1.F;
		arrow->stroke_color.a = 1.F;
	}
	co_await stagger( stage, arrows, 0.2s, [&]( Visual &arrow ) { return fade_in( stage, arrow, 0.4s ); } );
	co_await circumscribe( stage, pipeline.node( "model" ) );
	co_await move_by( stage, pipeline.node( "model" ), { 0.F, 1.8F }, 0.8s, ease_cubic_in_out );
	co_await move_by( stage, pipeline.node( "model" ), { 0.F, -1.8F }, 0.8s, ease_cubic_in_out );
	co_await wait_for( stage, 3s );
}

// A triangle with named corners, a right angle and another marked
Scene show_geometry( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();
	const Point a{ -4.F, -2.F };
	const Point b{ 2.F, -2.F };
	const Point c{ 2.F, 2.F };

	stage.visuals.add( make_polygon( theme, { a, b, c }, { .fill = Color{ palette.blue.r, palette.blue.g, palette.blue.b, 0.2F }, .labels = { "A", "B", "C" }, .dots = true } ) );
	stage.visuals.add( make_angle( theme, b, a, c, { .radius = 0.5F } ) );
	stage.visuals.add( make_angle( theme, a, b, c, { .radius = 1.2F, .color = palette.orange, .label = "θ" } ) );
	stage.visuals.add( make_labeled_brace( theme, a, b, "adjacent", FontStyle{ Intent::LABEL }, { .color = palette.muted, .left = false } ) );

	co_await wait_for( stage, 2s );
}

// Data charts: scatter plots (markers, and a line through them), and bars grouped and stacked
Scene show_charts( Stage &stage, Script & )
{
	Theme &theme = *stage.theme;

	// Points, one series joined by a line, another as crosses
	auto &plot_axes = stage.visuals.add( make_axes( theme, { .x = { 0, 10 }, .y = { 0, 8 }, .width = 7.F, .height = 4.5F, .grid = true, .x_label = "t", .y_label = "v" } ) );
	plot_axes.position = { -3.7F, 0.2F };
	auto &measured = plot_axes.add_plot( scatter( plot_axes, { { 1, 1.5 }, { 2, 2.4 }, { 3.5, 3.0 }, { 5, 5.2 }, { 6.5, 5.6 }, { 8, 7.1 }, { 9.5, 7.4 } }, { .line = JoinStyle::SMOOTH, .name = "measured" } ) );
	auto &noisy = plot_axes.add_plot( scatter( plot_axes, { { 1, 3.0 }, { 2.5, 2.2 }, { 4, 4.6 }, { 5.5, 3.1 }, { 7, 4.4 }, { 9, 5.0 } }, { .marker = MarkerShape::CROSS, .size = 0.14F, .name = "noisy" } ) );
	auto &peaks = plot_axes.add_plot( scatter( plot_axes, { { 2, 6.5 }, { 4, 7.4 }, { 6, 6.0 } }, { .marker = MarkerShape::TRIANGLE, .name = "peaks" } ) );
	add_legend( plot_axes, make_legend( theme, { legend_entry( measured ), legend_entry( noisy ), legend_entry( peaks ) } ), Anchor::BOTTOM_RIGHT );

	// Bars side by side
	auto &bars = stage.visuals.add( make_bar_chart( theme, {
		.categories = { "Q1", "Q2", "Q3", "Q4" },
		.series = { { "north", { 12, 19, 15, 22 } }, { "south", { 9, 14, 20, 17 } } },
		.width = 4.8F, .height = 4.F, .values = Range{ 0, 30 }, .value_labels = true } ) );
	bars.position = { 4.9F, 0.6F };
	add_legend( bars.axes(), make_legend( theme, legend_entries( bars ), { .horizontal = true } ), Anchor::TOP_LEFT );

	auto &title = stage.visuals.add( make_text( theme, "Points and bars", FontStyle{ Intent::TITLE } ) );
	move_to( title, { 0.F, 4.1F }, Anchor::TOP );

	co_await wait_for( stage, 2s );

	// Stacked, going across, in a color a category
	stage.visuals.clear();
	auto &stacked = stage.visuals.add( make_bar_chart( theme, {
		.categories = { "one", "two", "three" },
		.series = { { "a", { 3, 5, 2 } }, { "b", { 2, 1, 4 } }, { "c", { 1, 2, 3 } } },
		.stacked = true, .horizontal = true, .width = 6.F, .height = 3.5F, .value_labels = true } ) );
	stacked.position = { -4.F, 0.F };
	auto &signed_bars = stage.visuals.add( make_bar_chart( theme, {
		.categories = { "a", "b", "c", "d", "e" },
		.series = { { "change", { 4, -2, 3, -5, 1 } } },
		.width = 5.5F, .height = 4.F, .color_by_category = true } ) );
	signed_bars.position = { 4.5F, 0.F };
	co_await wait_for( stage, 2s );
}

// Tables: columns that wrap to a width, formulas as cells, tinted rows, and a grid
Scene show_tables( Stage &stage, Script & )
{
	using namespace math::literals;
	Theme &theme = *stage.theme;
	const Palette &palette = theme.palette();
	const FontStyle style{ Intent::PROSE };

	auto &title = stage.visuals.add( make_text( theme, "Tables", FontStyle{ Intent::TITLE } ) );
	move_to( title, { 0.F, 4.1F }, Anchor::TOP );

	// A table too wide for its place: the widest column is wrapped, and the rows tinted
	const std::vector<std::vector<std::string>> steps = {
		{ "Step", "What happens" },
		{ "read", "The input is read a line at a time and split into words, keeping the position of each" },
		{ "check", "Every word is looked up, and the ones that are not known are collected for the report" },
		{ "run", "Done" } };
	auto &notes = stage.visuals.add( make_panel( make_table(
		theme,
		steps,
		{ .striped = true, .header_fill = Color{ palette.blue.r, palette.blue.g, palette.blue.b, 0.25F }, .max_width = 6.6F } ) ) );
	move_to( notes, { -3.6F, 0.9F }, Anchor::CENTER );

	// Formulas in the cells, the columns lined up on their edges
	math::Context context( theme, FontStyle{ Intent::MATH } );
	auto cell = [&]( const math::Expr &formula )
	{
		auto visual = formula.build( context );
		return std::unique_ptr<Visual>( std::move( visual ) );
	};
	std::vector<std::vector<std::unique_ptr<Visual>>> rows( 4 );
	rows[0].push_back( make_text( theme, "function", FontStyle{ Intent::PROSE }.bold() ) );
	rows[0].push_back( make_text( theme, "derivative", FontStyle{ Intent::PROSE }.bold() ) );
	using namespace math;
	rows[1].push_back( cell( superscript( "x"_m, "2"_m ) ) );
	rows[1].push_back( cell( "2"_m * "x"_m ) );
	rows[2].push_back( cell( row( { op( sin() ), ord( "x"_m ) } ) ) );
	rows[2].push_back( cell( row( { op( cos() ), ord( "x"_m ) } ) ) );
	rows[3].push_back( cell( radical( "x"_m ) ) );
	rows[3].push_back( cell( "1"_m / ( "2"_m * radical( "x"_m ) ) ) );
	auto &formulas = stage.visuals.add( make_panel( make_table( theme, std::move( rows ), { .grid = true, .alignment = { Align::CENTER } } ) ) );
	move_to( formulas, { 4.2F, 0.6F }, Anchor::CENTER );

	co_await wait_for( stage, 3s );
}

// Music under the demo movie. Music is not in the repository (assets/ is not): without the file the movie
// plays silent, and says so.
constexpr const char *MOVIE_MUSIC = "assets/music/Alexander Ehlers - Spacetime.mp3";

// The whole demo: the scenes above one after another, each on a fresh stage, with music under them that fades
// in, and out at the end
Scene movie( Stage &stage, Script &script )
{
	Sound music;
	if ( std::filesystem::exists( MOVIE_MUSIC ) )
		music = stage.audio.play( MOVIE_MUSIC, { .volume = -22.0, .loop = true, .fade_in = 2s } );
	else
		std::cerr << "No music: " << MOVIE_MUSIC << " is not there\n";

	co_await sequence(
		stage,
		script,
		{ "morph-shapes", "morph-formula", "morph-text", "morph-strokes", "karaoke" },
		{ .fade_out = 0.4s, .fade_in = 0.4s } );

	music.fade_out( 3s );
	co_await wait_for( stage, 3s );
}

VOXIMA_PLUGIN();

REGISTER_SCENE( "missing-glyphs", "Characters that cannot be drawn, shown as magenta boxes (and reported)", show_missing_glyphs );
REGISTER_SCENE( "eases", "The ease functions, as circles racing across the screen (with narration)", show_eases );
REGISTER_SCENE( "formulas", "A page of laid-out math formulas", show_formulas );
REGISTER_SCENE( "code", "Syntax-highlighted C++, typed, with lines pointed out", show_code );
REGISTER_SCENE( "layout", "Panels, a list and a table, and a row of shapes, placed against each other", show_layout );
REGISTER_SCENE( "verbs", "Fade, slide, grow and indicate, with connectors following the moving circles", show_verbs );
REGISTER_SCENE( "diagram", "A diagram laid out in layers, a callout, a wrapped list, and evenly spread headings", show_diagram );
REGISTER_SCENE( "geometry", "A triangle with named corners and marked angles", show_geometry );
REGISTER_SCENE( "math-marks", "Cancelled and boxed parts, colors, primes, and words between math", show_math_marks );
REGISTER_SCENE( "charts", "Scatter plots and bar charts: grouped, stacked, horizontal, with negatives", show_charts );
REGISTER_SCENE( "tables", "Tables that wrap, hold formulas, and tint their rows", show_tables );
REGISTER_SCENE( "camera", "Zooming in on a graph, and panning along it", show_camera );
REGISTER_SCENE( "plot", "Axes crossing in the middle, and graphs drawn on them", show_plot );
REGISTER_SCENE( "plot-edge", "The same, with the axes along the left and bottom", show_plot_edge );
REGISTER_SCENE( "morph-shapes", "A circle turning into a square and letters, with holes opening and closing", show_morph );
REGISTER_SCENE( "morph-formula", "(a+b)^2 rewritten step by step, matching parts by tag and glyph", show_formula_morph );
REGISTER_SCENE( "morph-text", "A sentence edited word by word", show_text_morph );
REGISTER_SCENE( "morph-strokes", "A line, an arrow, staggered letters and a picture morphing", show_more_morphs );
REGISTER_SCENE( "karaoke", "Narrated text lit word by word, and a shape changing on the words", show_karaoke );
REGISTER_SCENE( "movie", "All the morph demos, then the karaoke", movie );
