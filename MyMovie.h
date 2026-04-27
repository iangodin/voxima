
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
#include "Image.h"
#include "Animation.h"
#include "CallbackVisitor.h"

#include <coroutine>

Font font( "Roboto-Regular.ttf" );

Director scale_in( Visual &visual, int64_t frames )
{
	co_await together(
		ease_in_out_cubic( visual.scale_x, 0.F, 1.F, frames ),
		ease_in_out_cubic( visual.scale_y, 0.F, 1.F, frames ) );
}

Director scale_x_in( Visual &visual, int64_t frames )
{
	co_await ease_in_out_cubic( visual.scale_x, 0.F, 1.F, frames );
}

Group &create_node( Stage &stage, std::string title, float x, float y, float w, float h )
{
	auto &group = stage.visuals.add( std::make_unique<Group>() );
	group.position = { x, y };

	auto &rect = group.add( std::make_unique<RoundRectangle>() );
	rect.position = { 0.F, 0.F };
	rect.half_width = w / 2.F;
	rect.half_height = h / 2.F;
	rect.color = { 0.1F, 0.6F, 0.2F, 1.0F };
	rect.stroke_thickness = 0.03f;
	rect.stroke_color = { 0.2f, 0.8f, 0.4f, 1.0f };
	rect.top_left_radius = 0.05,
	rect.top_right_radius = 0.05,
	rect.bottom_left_radius = 0.05,
	rect.bottom_right_radius = 0.05;

	auto &text = group.add( font.create_text( title, 0.3F ) );
	text.position = { 0.F, 0.F };
	text.color = { 1.F, 1.F, 1.F, 1.F };

	return group;
}

std::pair<Line &, ArrowTip &> create_arrow( Stage &stage, Point start, Point end )
{
	Point relative = end - start;

	auto &group = stage.visuals.add( std::make_unique<Group>() );
	group.position = start;

	auto &line = group.add( std::make_unique<Line>() );
	line.start = { 0.F, 0.F };
	line.end = relative;
	line.stroke_thickness = 0.05F;
	line.stroke_color = { 1.0F, 1.0F, 1.0F, 1.0F };

	auto &arrow = group.add( std::make_unique<ArrowTip>() );
	arrow.start = { 0.F, 0.F };
	arrow.end = relative;
	arrow.size = 0.08;
	arrow.stroke_thickness = 0.05F;
	arrow.stroke_color = { 1.0F, 1.0F, 1.0F, 1.0F };

	return { line, arrow };
}

Scene my_diagram( Stage &stage )
{
	auto &scene_node = create_node( stage, "Scene", -6.F, 0.F, 1.0F, 0.5F );
	co_await scale_in( scene_node, 12 );

	{
		auto [line, arrow] = create_arrow( stage, { -5.F, 0.F }, { -4.0, 0.F } );
		co_await together(
			ease_in_out_cubic( line.end.x, line.start.x, line.end.x, 12 ),
			ease_in_out_cubic( arrow.end.x, arrow.start.x, arrow.end.x, 12 ) );
	}

	auto &oetf_node = create_node( stage, "OETF", -3.F, 0.F, 1.0F, 0.5F );
	co_await scale_in( oetf_node, 12 );

	{
		auto [line, arrow] = create_arrow( stage, { -2.F, 0.F }, { -1.0, 0.F } );
		co_await together(
			ease_in_out_cubic( line.end.x, line.start.x, line.end.x, 12 ),
			ease_in_out_cubic( arrow.end.x, arrow.start.x, arrow.end.x, 12 ) );
	}

	auto &signal_node = create_node( stage, "Signal", 0.F, 0.F, 1.0F, 0.5F );
	co_await scale_in( signal_node, 12 );
}

Screenplay my_movie( Stage &stage )
{
	auto &img = stage.visuals.add( std::make_unique<Image>() );
	img.path = "testcard.png";
	img.position = { 4.0, 0.0 };
	img.half_width = 2.0;
	img.half_height = 2.0;

	co_await together(
		ease_in_out_cubic( img.rotation, -90.F, 0.F, 60 ),
		my_diagram( stage ) );
}

