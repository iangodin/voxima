// The demo's scenes. With no --scene the "movie" scene renders (the morph scenes, then the karaoke); any one can be
// rendered on its own, or several in order:
//
//     sonimation ... --scene morph-text
//     sonimation ... --scene morph-shapes --scene karaoke
//     sonimation --list-scenes
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
#include <visuals/Tags.h>
#include <visuals/Circle.h>
#include <visuals/Callout.h>
#include <visuals/Connector.h>
#include <visuals/Diagram.h>
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

Scene narrate( Narration &narration )
{
	while ( !narration.is_done() )
	{
		narration.speak();
		co_await next_frame();
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
	running.push_back( narrate( narration ) );
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
		narrate( narration ) );
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
	co_await wait_for( stage, 0.5s );

	auto &wave = axes.add_plot( plot( axes, []( double x ) { return std::sin( x ); } ) );
	co_await draw( stage, wave, 2s );

	GraphOptions orange;
	orange.color = stage.theme->palette().orange;
	auto &inverse = axes.add_plot( plot( axes, []( double x ) { return 1.0 / x; }, orange ) );
	co_await draw( stage, inverse, 2s );
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
	auto list_panel = make_panel( make_list( theme, { "Rows and columns", "Grids of any size", "Panels that fit", "Numbered lists too" } ) );
	auto table_panel = make_panel( make_table(
		theme,
		{ { "Palette", "Type", "Background" },
		  { "paper", "light", "warm white" },
		  { "one-dark", "dark", "slate" },
		  { "blueprint", "dark", "navy" } },
		{ .alignment = { Align::START, Align::CENTER, Align::END } } ) );
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
	auto circle = std::make_unique<Circle>();
	circle->radius = 0.5F;
	circle->color = palette.red;
	auto square = std::make_unique<Rectangle>();
	square->half_width = square->half_height = 0.5F;
	square->color = palette.blue;
	auto bar = std::make_unique<Rectangle>();
	bar->half_width = 1.1F;
	bar->half_height = 0.3F;
	bar->color = palette.green;
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

SONIMATION_SCENE( "missing-glyphs", "Characters that cannot be drawn, shown as magenta boxes (and reported)", show_missing_glyphs );
SONIMATION_SCENE( "eases", "The ease functions, as circles racing across the screen (with narration)", show_eases );
SONIMATION_SCENE( "formulas", "A page of laid-out math formulas", show_formulas );
SONIMATION_SCENE( "code", "Syntax-highlighted C++, typed, with lines pointed out", show_code );
SONIMATION_SCENE( "layout", "Panels, a list and a table, and a row of shapes, placed against each other", show_layout );
SONIMATION_SCENE( "verbs", "Fade, slide, grow and indicate, with connectors following the moving circles", show_verbs );
SONIMATION_SCENE( "diagram", "A diagram laid out in layers, a callout, a wrapped list, and evenly spread headings", show_diagram );
SONIMATION_SCENE( "camera", "Zooming in on a graph, and panning along it", show_camera );
SONIMATION_SCENE( "plot", "Axes crossing in the middle, and graphs drawn on them", show_plot );
SONIMATION_SCENE( "plot-edge", "The same, with the axes along the left and bottom", show_plot_edge );
SONIMATION_SCENE( "morph-shapes", "A circle turning into a square and letters, with holes opening and closing", show_morph );
SONIMATION_SCENE( "morph-formula", "(a+b)^2 rewritten step by step, matching parts by tag and glyph", show_formula_morph );
SONIMATION_SCENE( "morph-text", "A sentence edited word by word", show_text_morph );
SONIMATION_SCENE( "morph-strokes", "A line, an arrow, staggered letters and a picture morphing", show_more_morphs );
SONIMATION_SCENE( "karaoke", "Narrated text lit word by word, and a shape changing on the words", show_karaoke );
SONIMATION_SCENE( "movie", "All the morph demos, then the karaoke", movie );
