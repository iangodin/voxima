#pragma once

#include "visuals/Group.h"
#include "visuals/RoundRectangle.h"

#include <memory>

struct PanelOptions
{
	// Room round the content, and how rounded the corners are (stage units)
	float padding = 0.4F;
	float radius = 0.25F;

	// White is the theme's text color (see Color::resolved), so by default the panel is a faint tint of it with a softer edge:
	// light on dark, dark on light, on any palette
	Color fill = { 1.F, 1.F, 1.F, 0.08F };
	Color outline = { 1.F, 1.F, 1.F, 0.35F };
	float outline_width = 0.03F;
};

// A box round some content, fitted to it: a rounded rectangle behind the content, as big as its box (see box_of) and the padding.
// Move, scale or fade the Panel and both go with it. If the content changes size or moves, fit() again.
//
//     auto &panel = stage.visuals.add( make_panel( make_text( theme, "Theorem", style ), { .padding = 0.5F } ) );
class Panel : public Group
{
public:
	Panel( std::unique_ptr<Visual> content, const PanelOptions &options );

	RoundRectangle &background()
	{
		return *_background;
	}

	Visual &content()
	{
		return *_content;
	}

	const PanelOptions &options() const
	{
		return _options;
	}

	// Size and place the background round the content as it is now
	void fit();

private:
	PanelOptions _options;
	RoundRectangle *_background = nullptr;
	Visual *_content = nullptr;
};

std::unique_ptr<Panel> make_panel( std::unique_ptr<Visual> content, const PanelOptions &options = {} );
