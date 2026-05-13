

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

#include <visuals/CreateText.h>
#include <visuals/Circle.h>

#include <tts/Narration.h>

template<typename Ease>
Scene show_ease( Stage &stage, const std::string &name, Ease &&ease, float y, int64_t frames )
{
	float x1 = -4.F;
	float x2 = 7.F;
	auto &circle = stage.visuals.add( std::make_unique<Circle>() );
	circle.radius = 0.1F;
	circle.position = { x1, y };

	auto &label = create_text( stage, name, TextStyle{ Intent::LABEL } );
	label.position = { -6.F, y };

	co_await wait_for( 30 );
	co_await ease( circle.position.x, x1, x2, frames );
	co_await wait_for( 30 );
}

Scene narrate( Stage &stage, Script &script )
{
	tts::Narration narration;

	narration.synthesize( stage,
		script.root()["voices"]["narrator"].as_string(),
		script.root()["meta"]["lang"][0].as_string(),
		script.root()["intro"].as_multiline() );

	Microphone mic( stage.audio );
	while ( !narration.is_done() )
	{
		narration.speak( mic );
		co_await wait_for( 1 );
	}
}

Screenplay my_screenplay( Stage &stage, Script &script )
{
	int64_t duration = 60;

	std::string not_found{ "\uFFFD" };

	if ( !script.root().is_dict() )
	{
		throw std::runtime_error( "root is not dict");
	}
	std::cout << "Size: " << script.root().size() << '\n';
	for ( const auto &entry: script.root().as_dict() )
	{
		std::cout << "Entry: " << entry.first << '\n';
	}

	auto &labels = script.root()["labels"];
	if ( labels.is_missing() )
	{
		throw std::runtime_error( "labels are missing" );
	}

	std::string label_cubic_in{ labels["ease_cubic_in"].as_string( not_found ) };
	std::string label_cubic_out{ labels["ease_cubic_out"].as_string( not_found ) };
	std::string label_cubic_in_out{ labels["ease_cubic_in_out"].as_string( not_found ) };
	std::string label_elastic_in{ labels["ease_elastic_in"].as_string( not_found ) };
	std::string label_elastic_out{ labels["ease_elastic_out"].as_string( not_found ) };
	std::string label_elastic_in_out{ labels["ease_elastic_in_out"].as_string( not_found ) };
	std::string label_back_in{ labels["ease_back_in"].as_string( not_found ) };
	std::string label_back_out{ labels["ease_back_out"].as_string( not_found ) };
	std::string label_back_in_out{ labels["ease_back_in_out"].as_string( not_found ) };
	std::string label_linear{ labels["ease_linear"].as_string( not_found ) };

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

