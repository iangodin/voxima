

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

#include <visuals/Circle.h>
#include <visuals/CreateStyledText.h>
#include <visuals/CreateText.h>
#include <visuals/StyledText.h>
#include <visuals/CreateMathLargeOperation.h>
#include <visuals/Fraction.h>
#include <visuals/Radical.h>
#include <visuals/Delimited.h>
#include <visuals/LargeOperator.h>
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
	// 1. Get the math font from the current theme
	MathFonts math_fonts = stage.theme->get_math( FontStyle{ Intent::MATH }.large() );
	MathFont math_font( math_fonts.roman() );

	// 2. Determine scaling factors
	const float target_height_canvas = 3.5f;
	const float upem = static_cast<float>( math_fonts.roman().face->units_per_EM );
	const float font_size = math_fonts.roman().scale;
	const float glyph_scale = font_size / upem;

	// 3. Common large operators
	std::vector<char32_t> ops = {
//		U'∫', // Integral
//		U'∑', // Summation
		U'√' // Square Root
//		U'[', // Left Square Bracket
//		U']'  // Right Square Bracket
	};

	float x = -6.0f;
	const float spacing = 3.0f;

	auto style = FontStyle{ Intent::MATH }.large();
	auto fraction = std::make_unique<Fraction>(
		math_font,
		glyph_scale,
		make_text( *stage.theme, "22", style ),
		make_text( *stage.theme, "7", style ) );

	for ( char32_t op: ops )
	{
		float h = ( op == U']' ) ? 8.0f : target_height_canvas;
        if (op == U'√')
        {
            auto radical = std::make_unique<Radical>( math_font, glyph_scale,
				make_text( *stage.theme, "137", script_style( style, math_font, ScriptLevel::SCRIPT_SCRIPT ) ),
				std::move( fraction ) );
            radical->position = { x, 0.0f };
            stage.visuals.add(std::move(radical));
        }
        else
        {
            std::unique_ptr<Visual> visual = create_math_large_op( math_font, op, h / glyph_scale );
            *visual *= glyph_scale;
            visual->position = { x, 0.0f };
            visual->color = Color{ 0.2f, 0.6f, 1.0f, 1.0f };
            stage.visuals.add( std::move( visual ) );
        }
		x += spacing;
	}

	// A formula: x with scripts + 22/7 = 3.14, spaced by atom class
	auto script = script_style( style, math_font, ScriptLevel::SCRIPT );
	auto row = std::make_unique<Row>( math_font, glyph_scale );
	row->append(
		AtomClass::ORD,
		std::make_unique<Scripts>(
			math_font,
			glyph_scale,
			make_text( *stage.theme, "x", style ),
			make_text( *stage.theme, "2", script ),
			make_text( *stage.theme, "i", script ) ) );
	row->append( AtomClass::BIN, make_text( *stage.theme, "+", style ) );
	row->append(
		AtomClass::INNER,
		std::make_unique<Fraction>(
			math_font,
			glyph_scale,
			make_text( *stage.theme, "22", style ),
			make_text( *stage.theme, "7", style ) ) );
	row->append( AtomClass::REL, make_text( *stage.theme, "=", style ) );
	row->append( AtomClass::ORD, make_text( *stage.theme, "3.14", style ) );
	row->position = { -6.0f, -2.5f };
	stage.visuals.add( std::move( row ) );

	// A sum: limits above and below the operator
	auto sum = make_large_operator(
		math_font,
		glyph_scale,
		U'\u2211',
		make_text( *stage.theme, "\u221E", script ),
		make_text( *stage.theme, "n=1", script ) );
	sum->position = { 1.5f, 0.0f };
	stage.visuals.add( std::move( sum ) );

	// An integral in a formula: limits at the side of the operator
	auto upright = style;
	upright.upright();
	auto integral = std::make_unique<Row>( math_font, glyph_scale );
	integral->append(
		AtomClass::OP,
		make_large_operator(
			math_font,
			glyph_scale,
			U'\u222B',
			make_text( *stage.theme, "1", script ),
			make_text( *stage.theme, "0", script ) ) );
	integral->append( AtomClass::ORD, make_text( *stage.theme, "x", style ) );
	integral->append_space( Row::THIN );
	integral->append( AtomClass::ORD, make_text( *stage.theme, "d", upright ) );
	integral->append( AtomClass::ORD, make_text( *stage.theme, "x", style ) );
	integral->append( AtomClass::REL, make_text( *stage.theme, "=", style ) );
	integral->append(
		AtomClass::INNER,
		std::make_unique<Fraction>(
			math_font,
			glyph_scale,
			make_text( *stage.theme, "1", style ),
			make_text( *stage.theme, "2", style ) ) );
	integral->position = { -6.0f, 2.5f };
	stage.visuals.add( std::move( integral ) );

	// Delimiters that stretch to fit their content: a fraction in parentheses, and a binomial
	auto fenced = std::make_unique<Delimited>(
		math_font,
		glyph_scale,
		U'(',
		std::make_unique<Fraction>(
			math_font,
			glyph_scale,
			make_text( *stage.theme, "22", style ),
			make_text( *stage.theme, "7", style ) ),
		U')' );
	fenced->position = { 2.5f, 2.5f };
	stage.visuals.add( std::move( fenced ) );

	auto binomial = make_binomial(
		math_font,
		glyph_scale,
		make_text( *stage.theme, "n", style ),
		make_text( *stage.theme, "k", style ) );
	binomial->position = { 5.5f, 2.5f };
	stage.visuals.add( std::move( binomial ) );
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
