

#include <core/Stage.h>
#include <core/Coroutine.h>
#include <core/Screenplay.h>
#include <core/Scene.h>
#include <core/Director.h>
#include <core/Wait.h>

#include <visuals/CreateText.h>
#include <visuals/Circle.h>

#include <core/EaseCubic.h>
#include <core/EaseElastic.h>
#include <core/EaseBack.h>
#include <core/EaseLinear.h>

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

Screenplay my_screenplay( Stage &stage )
{
	int64_t duration = 60;
	co_await choreograph(
		show_ease( stage, "Cubic In", ease_cubic_in, 3.F, duration ),
		show_ease( stage, "Cubic Out", ease_cubic_out, 2.5F, duration ),
		show_ease( stage, "Cubic In Out", ease_cubic_in_out, 2.0F, duration ),
		show_ease( stage, "Elastic In", ease_elastic_in, 1.5F, duration ),
		show_ease( stage, "Elastic Out", ease_elastic_out, 1.0F, duration ),
		show_ease( stage, "Elastic In Out", ease_elastic_in_out, 0.5F, duration ),
		show_ease( stage, "Back In", ease_back_in, 0.F, duration ),
		show_ease( stage, "Back Out", ease_back_out, -0.5F, duration ),
		show_ease( stage, "Back In Out", ease_back_in_out, -1.0F, duration ),
		show_ease( stage, "Linear", ease_linear, -1.5F, duration )
	);
}

