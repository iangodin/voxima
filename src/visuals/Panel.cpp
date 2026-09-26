#include "visuals/Panel.h"

#include "visuals/Placement.h"

#include <algorithm>

Panel::Panel( std::unique_ptr<Visual> content, const PanelOptions &options ) :
	_options( options )
{
	// The background first: behind the content
	_background = &add( std::make_unique<RoundRectangle>() );
	_background->color = options.fill;
	_background->stroke_color = options.outline;
	_background->stroke_thickness = options.outline_width;

	_content = &add( std::move( content ) );
	fit();
}

void Panel::fit()
{
	const AABB box = box_of( *_content );
	if ( box.empty() )
	{
		_background->half_width = _background->half_height = 0.F;
		return;
	}

	const float half_width = ( box.max.x() - box.min.x() ) / 2.F + _options.padding;
	const float half_height = ( box.max.y() - box.min.y() ) / 2.F + _options.padding;
	_background->half_width = half_width;
	_background->half_height = half_height;
	_background->position = { ( box.min.x() + box.max.x() ) / 2.F, ( box.min.y() + box.max.y() ) / 2.F };

	// A corner is never rounder than the panel is small
	const float radius = std::min( { _options.radius, half_width, half_height } );
	_background->top_left_radius = _background->top_right_radius = radius;
	_background->bottom_left_radius = _background->bottom_right_radius = radius;
}

std::unique_ptr<Panel> make_panel( std::unique_ptr<Visual> content, const PanelOptions &options )
{
	return std::make_unique<Panel>( std::move( content ), options );
}
