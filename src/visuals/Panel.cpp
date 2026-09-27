#include "visuals/Panel.h"

#include "visuals/Placement.h"

#include <algorithm>

Panel::Panel( std::unique_ptr<Visual> content, const PanelOptions &options ) :
	_options( options )
{
	// The shadow, if there is one, at the very back
	if ( options.shadow )
	{
		_shadow = &add( std::make_unique<Group>() );
		const int layers = std::max( 1, options.shadow->layers );
		for ( int i = 0; i < layers; ++i )
		{
			auto layer = std::make_unique<RoundRectangle>();
			layer->color = options.shadow->color;
			layer->color.a = options.shadow->color.a / static_cast<float>( layers );
			_shadow->add( std::move( layer ) );
		}
	}

	// The background next: behind the content
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
		if ( _shadow )
		{
			for ( const auto &layer: *_shadow )
			{
				static_cast<RoundRectangle &>( *layer ).half_width = static_cast<RoundRectangle &>( *layer ).half_height = 0.F;
			}
		}
		extent = AABB{};
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

	if ( _shadow )
	{
		const ShadowOptions &shadow = *_options.shadow;
		const auto layers = static_cast<float>( _shadow->size() );
		float i = 0.F;
		for ( const auto &child: *_shadow )
		{
			auto &layer = static_cast<RoundRectangle &>( *child );
			// From a little inside the panel's edge to a little outside it, the fade centered on the edge
			const float spread = shadow.blur * ( ( i + 0.5F ) / layers - 0.5F );
			layer.half_width = std::max( 0.F, half_width + spread );
			layer.half_height = std::max( 0.F, half_height + spread );
			layer.top_left_radius = layer.top_right_radius = std::max( 0.F, radius + spread );
			layer.bottom_left_radius = layer.bottom_right_radius = layer.top_left_radius;
			layer.position = _background->position + shadow.offset;
			i += 1.F;
		}
	}

	// The panel occupies its background's box for layout purposes: a shadow is a visual effect behind it, not
	// something else should make room for, so it must not move where the panel is anchored or placed next to.
	extent = AABB{ _background->position - Point{ half_width, half_height }, _background->position + Point{ half_width, half_height } };
}

std::unique_ptr<Panel> make_panel( std::unique_ptr<Visual> content, const PanelOptions &options )
{
	return std::make_unique<Panel>( std::move( content ), options );
}
