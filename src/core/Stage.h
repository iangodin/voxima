
#pragma once

#include <visuals/Group.h>
#include <visuals/Color.h>

#include <audio/DubbingStage.h>

#include <exporters/SVGWriter.h>

#include <core/Theme.h>
#include <core/Camera.h>
#include <core/Palette.h>
#include <core/Updaters.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

struct Stage
{
	Theme *theme = nullptr;
	Color background = { 0.F, 0.F, 0.F, 1.F };

	// What pure white is drawn as: the text color (see Color::resolved). White, the default, is as it has always been.
	Color foreground = { 1.F, 1.F, 1.F, 1.F };

	// The palette's background and text color, for the picture
	void use_palette( const Palette &palette )
	{
		background = palette.background;
		foreground = palette.text;
	}
	Group visuals;

	// Things run every frame to keep one thing tied to another (see Updaters.h): after the scene has done its step, before the
	// picture. Cleared, with the visuals, when a scene starts.
	Updaters updaters;
	DubbingStage audio;
	Camera camera;

	// Told when a scene played with play() (see SceneRegistry.h) starts (true) and ends (false), each in the
	// tick that does it, so the producer can note the frames it spans. Left empty, nothing is told.
	std::function<void( const std::string &name, bool started )> scene_event;

	void save_svg( std::ostream &file ) const
	{
		if ( !file ) return;

		// What the camera sees: its zoom sets how many stage units the picture is across, and its center which part
		// (the picture is drawn upside down, so y flips in the view: see the scale(1, -1) below)
		const float view_width = camera.view_width();
		const float view_height = camera.view_height();
		const float left = camera.center.x() - view_width / 2.F;
		const float top = -camera.center.y() - view_height / 2.F;

		// Curves drawn to within a twenty-fifth of a pixel (see SVGWriter), so a shape looks the same however its
		// outline is cut up: a glyph, and the morph that turns into it
		const float pixel = view_width / static_cast<float>( camera.width );

		std::ostringstream body;
		SVGWriter writer( body, 2, pixel / 25.F, foreground );
		visuals.accept( writer );

		file << "<svg width=\"" << camera.width << "\" height=\"" << camera.height << "\" "
		     << "viewBox=\"" << left << " " << top << " " << view_width << " " << view_height << "\" "
		     << "xmlns=\"http://www.w3.org/2000/svg\">\n";

		writer.write_defs( file );

		// Background, over all that is seen
		file << "  <rect x=\"" << left << "\" y=\"" << top << "\" width=\"" << view_width << "\" height=\"" << view_height << "\" "
		     << SVGWriter::color_attributes( background ) << " />\n";

		// Flip the Y-axis for all visuals
		file << "  <g transform=\"scale(1, -1)\">\n";

		file << body.str();

		file << "  </g>\n";
		file << "</svg>\n";
	}
};
