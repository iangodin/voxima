

#include <core/Stage.h>
#include <core/Script.h>
#include <core/Coroutine.h>
#include <core/Screenplay.h>
#include <core/Scene.h>
#include <core/Director.h>
#include <core/Wait.h>
#include <core/EaseCubic.h>
#include <core/EaseElastic.h>
#include <core/EaseBack.h>
#include <core/EaseLinear.h>

#include <visuals/StyledText.h>
#include <visuals/CreateText.h>
#include <visuals/CreateStyledText.h>
#include <visuals/Circle.h>

#include <core/Narration.h>

void show_title( Stage &stage )
{
	StyledText text( "Hello\nWorld" );
	text.set_font_style( 0, text.size(), FontStyle{ Intent::TITLE } );
	text.set_color( 0, 5, Color{ 1.0f, 0.0f, 0.0f, 1.0f } );
	text.set_weight( 2, 4, Weight::BLACK );

	auto &label = create_styled_text( stage, text );
	label.position = { 3.F, 0.F };
}

template<typename Ease>
Scene show_ease( Stage &stage, const std::string &name, Ease &&ease, float y, int64_t frames )
{
	float x1 = -4.F;
	float x2 = 7.F;
	auto &circle = stage.visuals.add( std::make_unique<Circle>() );
	circle.radius = 0.1F;
	circle.position = { x1, y };

	auto &label = create_text( stage, name, FontStyle{ Intent::LABEL } );
	label.position = { -6.F, y };

	co_await wait_for( 30 );
	co_await ease( circle.position.x, x1, x2, frames );
	co_await wait_for( 30 );
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
	int64_t duration = 60;

	std::string not_found{ "\uFFFD" };

	const std::string lang = "en";
	std::string label_cubic_in{ script.fetch( lang, "labels", "ease_cubic_in" ).as_string( not_found ) };
	std::string label_cubic_out{ script.fetch( lang, "labels", "ease_cubic_out" ).as_string( not_found ) };
	std::string label_cubic_in_out{ script.fetch( lang, "labels", "ease_cubic_in_out" ).as_string( not_found ) };
	std::string label_elastic_in{ script.fetch( lang, "labels", "ease_elastic_in" ).as_string( not_found ) };
	std::string label_elastic_out{ script.fetch( lang, "labels", "ease_elastic_out" ).as_string( not_found ) };
	std::string label_elastic_in_out{ script.fetch( lang, "labels", "ease_elastic_in_out" ).as_string( not_found ) };
	std::string label_back_in{ script.fetch( lang, "labels", "ease_back_in" ).as_string( not_found ) };
	std::string label_back_out{ script.fetch( lang, "labels", "ease_back_out" ).as_string( not_found ) };
	std::string label_back_in_out{ script.fetch( lang, "labels", "ease_back_in_out" ).as_string( not_found ) };
	std::string label_linear{ script.fetch( lang, "labels", "ease_linear" ).as_string( not_found ) };

	show_title( stage );

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
		narrate( stage, script )
	);
}

