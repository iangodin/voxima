
#include "Production.h"
#include "Stage.h"
#include "Scene.h"
#include "Producer.h"
#include "Screenplay.h"
#include "Director.h"
#include "Rectangle.h"
#include "RoundRectangle.h"
#include "Path.h"
#include "Font.h"
#include "Animation.h"

#include <coroutine>

Director background_rain( Stage &stage, int64_t frames )
{
	while ( frames > 0 )
	{
		stage.message += ".";
		--frames;
		co_yield {};
		}
}

Director background_snow( Stage &stage, int64_t frames )
{
	while ( frames > 0 )
	{
		stage.message += "o";
		--frames;
		co_yield {};
		}
}

Scene my_introduction( Stage &stage )
{
	stage.message = "My introduction";
	co_await wait_for( 5 );

	co_return;
}

Scene my_show( Stage &stage )
{
	stage.message = "My show";

	Font font( "Roboto-Regular.ttf" );
	auto text_group = font.create_text( "Hello!", 2.0f );
	text_group->position = { -4.0f, 2.0f };
	text_group->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	
	auto &text = stage.visuals.add( std::move( text_group ) );

	auto &rect = stage.visuals.add( std::make_unique<Rectangle>() );
	rect.position = { 0.0f, 0.0f };
	rect.rotation = 0.0f;
	rect.half_width = 2.0f;
	rect.half_height = 1.0f;
	rect.color = { 0.0f, 0.8f, 0.0f, 1.0f }; // Nice green
	rect.stroke_thickness = 0.1f;
	rect.stroke_color = { 1.0f, 1.0f, 1.0f, 1.0f }; // White stroke

	auto &round_rect = stage.visuals.add( std::make_unique<RoundRectangle>() );
	round_rect.position = { 4.0f, 0.0f };
	round_rect.half_width = 1.0f;
	round_rect.half_height = 1.0f;
	round_rect.top_left_radius = 0.5f;
	round_rect.top_right_radius = 0.0f; // Sharp
	round_rect.bottom_left_radius = 0.2f;
	round_rect.bottom_right_radius = 0.8f;
	round_rect.color = { 0.8f, 0.2f, 0.2f, 1.0f }; // Reddish
	round_rect.stroke_thickness = 0.05f;
	round_rect.stroke_color = { 0.0f, 0.0f, 0.0f, 1.0f }; // Black stroke

	int64_t frames = 20;

	co_await together(
//		ease_out_cubic( text.position.x, -10.0f, 0.0f, frames ),
		ease_out_cubic( text.rotation, -90.0f, 0.0f, frames ),
		ease_in_out_cubic( rect.rotation, 0.0f, 90.0f, frames )
	);

    co_return;
}

Scene my_ending( Stage &stage )
{
	stage.message = "My ending";
	co_await together(
		background_rain( stage, 3 ),
		background_snow( stage, 1 ),
		wait_for( 5 )
	);

	stage.message = "Plot twist!";

    co_return;
}

Scene my_credits( Stage &stage )
{
	stage.message = "Credits rolling";
	co_await wait_for( 5 );

    co_return;
}

Screenplay my_movie( Stage &stage )
{
//	co_await my_introduction( stage );
	co_await my_show( stage );
//	co_await my_ending( stage );
//	co_await my_credits( stage );
};
