

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
#include <core/MorphAnim.h>
#include <visuals/Rectangle.h>

void show_title( Stage &stage )
{
	StyledText text( "Hello\nالعالم" );
	text.set_font_style( 0, text.size(), FontStyle{ Intent::TITLE }.large() );
	text.set_color( 0, 5, Color{ 1.0f, 0.0f, 0.0f, 1.0f } );
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
	find_tag( root_formula, "numerator" )->color = Color{ 1.F, 0.4F, 0.4F, 1.F };
	find_tag( root_formula, "denominator" )->color = Color{ 0.4F, 0.7F, 1.F, 1.F };
	find_tag( root_formula, "sign" )->color = Color{ 1.F, 0.85F, 0.3F, 1.F };
	find_tag( root_formula, "index" )->color = Color{ 0.5F, 1.F, 0.5F, 1.F };

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
	find_tag( equation, "ratio", "bar" )->color = Color{ 1.F, 0.85F, 0.3F, 1.F };
	find_tag( equation, "power", "superscript" )->color = Color{ 1.F, 0.4F, 0.4F, 1.F };

	// A sum: in display style its limits go over and under it
	place( sum( text( "n=1" ), text( "∞" ) ) );

	// A matrix; its entries can be reached the same way, by row and column
	MatrixOptions tight{ .column_sep_em = 0.6F };
	std::vector<std::vector<Expr>> grid = { { text( "a" ), text( "b" ) }, { text( "c" ), text( "d" ) } };
	auto &matrix_formula = place( pmatrix( grid, tight ) );
	find_tag( matrix_formula, "r0c1" )->color = Color{ 0.5F, 1.F, 0.5F, 1.F }; // the entry b

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
	Stage &stage, const std::string &name, const std::string &lang, Ease &&ease, float y, int64_t frames )
{
	float x1 = -4.F;
	float x2 = 7.F;
	auto &circle = stage.visuals.add( std::make_unique<Circle>() );
	circle.radius = 0.1F;
	circle.position = { x1, y };

	auto &label = stage.visuals.add( make_text( *stage.theme, name, FontStyle{ Intent::LABEL }, lang ) );
	label.position = { -6.F, y };

	auto anim = ease( x1, x2, frames );
	while ( anim.next() )
	{
		circle.position = { anim.value(), circle.position.y() };
		co_await next_frame();
	}

}

std::unique_ptr<Circle> demo_circle( Point at )
{
	auto c = std::make_unique<Circle>();
	c->radius = 0.8F;
	c->position = at;
	c->color = Color{ 1.F, 0.45F, 0.45F, 1.F };
	return c;
}

std::unique_ptr<Rectangle> demo_square( Point at )
{
	auto r = std::make_unique<Rectangle>();
	r->half_width = r->half_height = 0.7F;
	r->position = at;
	r->color = Color{ 0.4F, 0.7F, 1.F, 1.F };
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

// A shape that turns into other shapes and letters, one after the other: circle, square, then glyphs
// (with holes that open and close) and back to the circle.
Scene show_morph( Stage &stage )
{
	const Point home = { 0.F, 2.5F };
	auto circle = [&]() { return demo_circle( home ); };
	auto square = [&]() { return demo_square( home ); };
	auto letter = [&]( const char *s, const Color &color ) { return demo_letter( stage, s, color, home ); };

	Visual *shown = &stage.visuals.add( circle() );
	co_await wait_for( 12 );

	std::vector<std::unique_ptr<Visual>> targets;
	targets.push_back( square() );
	targets.push_back( letter( "a", Color{ 1.F, 0.85F, 0.3F, 1.F } ) );
	targets.push_back( letter( "e", Color{ 0.5F, 1.F, 0.5F, 1.F } ) );
	targets.push_back( letter( "8", Color{ 1.F, 1.F, 1.F, 1.F } ) );
	targets.push_back( letter( "x", Color{ 0.4F, 0.7F, 1.F, 1.F } ) );
	targets.push_back( circle() );

	for ( auto &target: targets )
	{
		Visual *next = target.get();
		co_await morph( stage, *shown, std::move( target ), 36, ease_cubic_in_out );
		shown = next;
		co_await wait_for( 12 );
	}
}

// A formula that is rewritten step by step. Parts that stay the same (the a, the b, the plus signs) only
// slide into place, the rest morph or fade: leaves are matched by their tags, then by identical glyphs.
Scene show_formula_morph( Stage &stage )
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
	co_await wait_for( 12 );

	std::vector<std::unique_ptr<Visual>> steps;
	steps.push_back( build( expanded ) );
	steps.push_back( build( radical( expanded ) ) );

	for ( auto &step: steps )
	{
		Visual *next = step.get();
		co_await morph( stage, *shown, std::move( step ), 40, ease_cubic_in_out );
		shown = next;
		co_await wait_for( 12 );
	}
}

Scene narrate( Narration &narration )
{
	while ( !narration.is_done() )
	{
		narration.speak();
		co_await wait_for( 1 );
	}
}

// A shape that changes on the words being said: a circle turns into a square over "show various ease", into
// an a on "animations", and back into a circle over "light up". Each change takes exactly as long as the
// narrator takes to say those words, whatever speed the voice happens to have.
Scene follow_speech( Stage &stage, Narration &narration )
{
	const Point spot = { 5.5F, 2.5F };
	auto smooth = []( float t ) { return t * t * ( 3.F - 2.F * t ); };

	Visual *shown = &stage.visuals.add( demo_circle( spot ) );

	auto square = demo_square( spot );
	Visual *next = square.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( square ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_words( 1, 3, [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
	shown = next;

	auto letter = demo_letter( stage, "a", Color{ 1.F, 0.85F, 0.3F, 1.F }, spot );
	next = letter.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( letter ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_words( 4, 4, [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
	shown = next;

	auto circle = demo_circle( spot );
	next = circle.get();
	co_await morph_driven(
		stage,
		*shown,
		std::move( circle ),
		MorphOptions{},
		[&]( Morph &m ) { return narration.track_words( 8, 9, [&m, smooth]( float t ) { m.set( smooth( t ) ); } ); } );
}

// Karaoke: shows the narrated text dimmed, then lights up each word as it is
// spoken (the word being spoken in yellow, words already spoken in white).
Scene show_karaoke( Stage &stage, Script &script )
{
	const std::string lang = script.master_language();

	// The text must be the narrated lines as written: word N spoken is word N here.
	std::string shown;
	for ( const auto &line: script.fetch( lang, "intro" ).as_multiline() )
	{
		shown += ( shown.empty() ? "" : "\n" ) + line;
	}

	StyledText styled( shown );
	styled.set_font_style( 0, styled.size(), FontStyle{ Intent::PROSE }.large() );
	styled.set_language( 0, styled.size(), lang );

	auto &text = stage.visuals.add( make_styled_text( *stage.theme, styled, TextLayout{ .align = TextAlign::CENTER } ) );
	center_group( text );
	text.position = { 0.F, 0.5F };

	const Color unspoken{ 0.45F, 0.45F, 0.45F, 1.F };
	const Color spoken{ 1.F, 1.F, 1.F, 1.F };
	const Color speaking{ 1.F, 0.85F, 0.3F, 1.F };

	for ( size_t i = 0; find_word( text, i ); ++i )
	{
		find_word( text, i )->color = unspoken;
	}

	Narration narration;
	narration.setup( "narrator", script, stage.audio );
	narration.speech( "intro" );

	Visual *current = nullptr;
	co_await choreograph(
		narrate( narration ),
		follow_speech( stage, narration ),
		narration.on_words(
			text,
			[&current, spoken, speaking]( Visual &word, size_t )
			{
				if ( current )
				{
					current->color = spoken;
				}
				word.color = speaking;
				current = &word;
			} ) );
}

Screenplay my_screenplay( Stage &stage, Script &script )
{
	int64_t duration = 24;

	std::string not_found{ "\uFFFD" };

	const std::string lang = script.master_language();
	std::string label_cubic_in{
		script.fetch( lang, "labels", "ease_cubic_in" ).as_string( not_found ) };
	std::string label_cubic_out{
		script.fetch( lang, "labels", "ease_cubic_out" ).as_string( not_found ) };
	std::string label_cubic_in_out{
		script.fetch( lang, "labels", "ease_cubic_in_out" ).as_string( not_found ) };
	std::string label_elastic_in{
		script.fetch( lang, "labels", "ease_elastic_in" ).as_string( not_found ) };
	std::string label_elastic_out{
		script.fetch( lang, "labels", "ease_elastic_out" ).as_string( not_found ) };
	std::string label_elastic_in_out{
		script.fetch( lang, "labels", "ease_elastic_in_out" ).as_string( not_found ) };
	std::string label_back_in{
		script.fetch( lang, "labels", "ease_back_in" ).as_string( not_found ) };
	std::string label_back_out{
		script.fetch( lang, "labels", "ease_back_out" ).as_string( not_found ) };
	std::string label_back_in_out{
		script.fetch( lang, "labels", "ease_back_in_out" ).as_string( not_found ) };
	std::string label_linear{
		script.fetch( lang, "labels", "ease_linear" ).as_string( not_found ) };

	//show_title( stage );
	// show_math( stage ); // shares the screen with the karaoke text below

	/*
	co_await choreograph(
		show_ease( stage, label_cubic_in, lang, ease_cubic_in, 3.F, duration ),
		show_ease( stage, label_cubic_out, lang, ease_cubic_out, 2.5F, duration ),
		show_ease( stage, label_cubic_in_out, lang, ease_cubic_in_out, 2.0F, duration ),
		show_ease( stage, label_elastic_in, lang, ease_elastic_in, 1.5F, duration ),
		show_ease( stage, label_elastic_out, lang, ease_elastic_out, 1.0F, duration ),
		show_ease( stage, label_elastic_in_out, lang, ease_elastic_in_out, 0.5F, duration ),
		show_ease( stage, label_back_in, lang, ease_back_in, 0.F, duration ),
		show_ease( stage, label_back_out, lang, ease_back_out, -0.5F, duration ),
		show_ease( stage, label_back_in_out, lang, ease_back_in_out, -1.0F, duration ),
		show_ease( stage, label_linear, lang, ease_linear, -1.5F, duration ),
		narrate( stage, script ) );
	*/
	co_await show_morph( stage );
	co_await show_formula_morph( stage );
	co_await show_karaoke( stage, script );
	co_await wait_for( duration );
}
