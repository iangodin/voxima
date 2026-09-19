

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
	auto place = [&]( const Expr &formula, float x, float y ) -> Visual &
	{
		auto visual = formula.build( context );
		visual->position = { x, y };
		return stage.visuals.add( std::move( visual ) );
	};

	// The integral of x dx from 0 to 1 is 1/2
	place(
		row( { op( integral( text( "0" ), text( "1" ) ) ),
		       ord( "x" ),
		       space( Row::THIN ),
		       ord( upright( "d" ) ),
		       ord( "x" ),
		       rel( "=" ),
		       inner( fraction( text( "1" ), text( "2" ) ) ) } ),
		-6.0F,
		2.5F );

	// A root with a wide index, around a fraction
	// Parts can be found again after building, to color or animate them. The composites name
	// their own parts, so none of this needs a tag of ours.
	auto &root_formula = place( root( text( "137" ), fraction( text( "22" ), text( "7" ) ) ), -6.0F, 0.0F );
	find_tag( root_formula, "numerator" )->color = Color{ 1.F, 0.4F, 0.4F, 1.F };
	find_tag( root_formula, "denominator" )->color = Color{ 0.4F, 0.7F, 1.F, 1.F };
	find_tag( root_formula, "sign" )->color = Color{ 1.F, 0.85F, 0.3F, 1.F };
	find_tag( root_formula, "index" )->color = Color{ 0.5F, 1.F, 0.5F, 1.F };

	// x^2_i + 22/7 = 3.14
	auto &equation = place(
		row( { ord( tag( "power", scripts( text( "x" ), text( "2" ), text( "i" ) ) ) ),
		       bin( "+" ),
		       inner( tag( "ratio", fraction( text( "22" ), text( "7" ) ) ) ),
		       rel( "=" ),
		       ord( "3.14" ) } ),
		-6.0F,
		-2.5F );
	// A path: the bar inside the part tagged "ratio", the superscript inside "power"
	find_tag( equation, "ratio", "bar" )->color = Color{ 1.F, 0.85F, 0.3F, 1.F };
	find_tag( equation, "power", "superscript" )->color = Color{ 1.F, 0.4F, 0.4F, 1.F };

	// A sum, a fraction in parentheses, a binomial
	place( sum( text( "n=1" ), text( "\u221E" ) ), 1.5F, 0.0F );
	place( parens( fraction( text( "22" ), text( "7" ) ) ), 2.5F, 2.5F );
	place( binomial( text( "n" ), text( "k" ) ), 5.5F, 2.5F );

	// Accents and rules
	place(
		row( { ord( hat( text( "x" ) ) ),
		       space( Row::THIN ),
		       ord( tilde( text( "y" ) ) ),
		       space( Row::THIN ),
		       ord( vec( text( "v" ) ) ),
		       space( Row::THIN ),
		       ord( hat( text( "A" ) ) ),
		       space( Row::THIN ),
		       ord( overline( text( "AB" ) ) ),
		       space( Row::THIN ),
		       ord( underline( text( "xy" ) ) ) } ),
		1.5F,
		-2.5F );

	// A matrix, and the same grid as a determinant
	MatrixOptions tight{ .column_sep_em = 0.6F };
	std::vector<std::vector<Expr>> grid = { { text( "a" ), text( "b" ) }, { text( "c" ), text( "d" ) } };
	auto &matrix_formula = place( pmatrix( grid, tight ), 3.0F, 0.0F );
	find_tag( matrix_formula, "r0c1" )->color = Color{ 0.5F, 1.F, 0.5F, 1.F }; // the entry b
	place( vmatrix( grid, tight ), 5.7F, 0.0F );
}

template<typename Ease>
Scene show_ease( Stage &stage, const std::string &name, Ease &&ease, float y, int64_t frames )
{
	float x1 = -4.F;
	float x2 = 7.F;
	auto &circle = stage.visuals.add( std::make_unique<Circle>() );
	circle.radius = 0.1F;
	circle.position = { x1, y };

	auto &label = stage.visuals.add( make_text( *stage.theme, name, FontStyle{ Intent::LABEL } ) );
	label.position = { -6.F, y };

	auto anim = ease( x1, x2, frames );
	while ( anim.next() )
	{
		circle.position = { anim.value(), circle.position.y() };
		co_await next_frame();
	}

}

Scene narrate( Stage &stage, Script &script )
{
	Narration narration;
	narration.setup( "narrator", script, stage.audio );
	narration.speech( "intro" );
	while ( !narration.is_done() )
	{
		narration.speak();
		co_await wait_for( 1 );
	}
}

Screenplay my_screenplay( Stage &stage, Script &script )
{
	int64_t duration = 24;

	std::string not_found{ "\uFFFD" };

	const std::string lang = "en";
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
	show_math( stage );

	/*
	co_await choreograph(
		show_ease( stage, label_cubic_in, ease_cubic_in, 3.F, duration ),
		show_ease( stage, label_cubic_out, ease_cubic_out, 2.5F, duration ),
		show_ease( stage, label_cubic_in_out, ease_cubic_in_out, 2.0F, duration ),
		show_ease( stage, label_elastic_in, ease_elastic_in, 1.5F, duration ),
		show_ease( stage, label_elastic_out, ease_elastic_out, 1.0F, duration ),
		show_ease( stage, label_elastic_in_out, ease_elastic_in_out, 0.5F, duration ),
		show_ease( stage, label_back_in, ease_back_in, 0.F, duration ),
		show_ease( stage, label_back_out, ease_back_out, -0.5F, duration ),
		show_ease( stage, label_back_in_out, ease_back_in_out, -1.0F, duration ),
		show_ease( stage, label_linear, ease_linear, -1.5F, duration ),
		narrate( stage, script ) );
	*/
	co_await wait_for( duration );
}
