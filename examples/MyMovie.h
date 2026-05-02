

#include "visuals/Rectangle.h"
#include "visuals/RoundRectangle.h"
#include "visuals/Path.h"
#include "visuals/CreateText.h"
#include "visuals/CreateMath.h"
#include "visuals/Image.h"
#include "visuals/Ellipse.h"
#include "visuals/CallbackVisitor.h"

#include "themes/NewComputerModern.h"

#include "core/EaseCubic.h"
#include "core/Production.h"
#include "core/Stage.h"
#include "core/Scene.h"
#include "core/Producer.h"
#include "core/Screenplay.h"
#include "core/Director.h"
#include "core/Wait.h"

#include <coroutine>

Animation scale_in( Visual &visual, int64_t frames )
{
	co_await choreograph(
		ease_in_out_cubic( visual.scale_x, 0.F, 1.F, frames ),
		ease_in_out_cubic( visual.scale_y, 0.F, 1.F, frames ) );
}

Animation scale_x_in( Visual &visual, int64_t frames )
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
	rect.top_left_radius = 0.05f,
	rect.top_right_radius = 0.05f,
	rect.bottom_left_radius = 0.05f,
	rect.bottom_right_radius = 0.05f;
	auto &text = create_text( group, stage.theme, title, TextStyle( "prose" ) );
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
	arrow.size = 0.08f;	arrow.stroke_thickness = 0.05F;
	arrow.stroke_color = { 1.0F, 1.0F, 1.0F, 1.0F };

	return { line, arrow };
}

Scene my_diagram( Stage &stage )
{
	auto &scene_node = create_node( stage, "場面", -6.F, 0.F, 1.0F, 0.5F );

	co_await scale_in( scene_node, 12 );

	{
		auto [line, arrow] = create_arrow( stage, { -5.F, 0.F }, { -4.0, 0.F } );
		co_await choreograph(
			ease_in_out_cubic( line.end.x, line.start.x, line.end.x, 12 ),
			ease_in_out_cubic( arrow.end.x, arrow.start.x, arrow.end.x, 12 ) );
	}

	auto &oetf_node = create_node( stage, "😎😱💩👽", -3.F, 0.F, 1.0F, 0.5F );
	co_await scale_in( oetf_node, 12 );

	{
		auto [line, arrow] = create_arrow( stage, { -2.F, 0.F }, { -1.0, 0.F } );
		co_await choreograph(
			ease_in_out_cubic( line.end.x, line.start.x, line.end.x, 12 ),
			ease_in_out_cubic( arrow.end.x, arrow.start.x, arrow.end.x, 12 ) );
	}

	auto &signal_node = create_node( stage, "Signal", 0.F, 0.F, 1.0F, 0.5F );
	co_await scale_in( signal_node, 12 );
}

Screenplay my_movie( Stage &stage )
{
	stage.theme = NewComputerModern::create_theme();

	auto &ellipse = stage.visuals.add( std::make_unique<Ellipse>() );
	ellipse.position = { 4.0, 0.0 };
	ellipse.half_width = 2.0;
	ellipse.half_height = 1.0;

	co_await wait_for( 30 );

	co_await choreograph(
		ease_in_out_cubic( ellipse.rotation, -90.F, 0.F, 60 ),
		my_diagram( stage ) );
}

