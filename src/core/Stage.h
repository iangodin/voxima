
#pragma once

#include <visuals/Group.h>
#include <visuals/Color.h>

#include <audio/DubbingStage.h>

#include <exporters/SVGWriter.h>

#include <core/Theme.h>
#include <core/Camera.h>

#include <functional>
#include <iostream>
#include <sstream>
#include <string>

struct Stage
{
	Theme *theme = nullptr;
	Color background = { 0.F, 0.F, 0.F, 1.F };
	Group visuals;
	DubbingStage audio;
	Camera camera;

	// Told when a scene played with play() (see SceneRegistry.h) starts (true) and ends (false), each in the
	// tick that does it, so the producer can note the frames it spans. Left empty, nothing is told.
	std::function<void( const std::string &name, bool started )> scene_event;

	void save_svg( std::ostream &file ) const
	{
		if ( !file ) return;

		float aspect = static_cast<float>( camera.height ) / static_cast<float>( camera.width );
		float view_width = 16.f;

		// Curves drawn to within a twenty-fifth of a pixel (see SVGWriter), so a shape looks the same however its
		// outline is cut up: a glyph, and the morph that turns into it
		const float pixel = view_width / static_cast<float>( camera.width );

		std::ostringstream body;
		SVGWriter writer( body, 2, pixel / 25.F );
		visuals.accept( writer );
		float view_height = view_width * aspect;

		file << "<svg width=\"" << camera.width << "\" height=\"" << camera.height << "\" "
		     << "viewBox=\"" << -view_width/2 << " " << -view_height/2 << " " << view_width << " " << view_height << "\" "
		     << "xmlns=\"http://www.w3.org/2000/svg\">\n";

		writer.write_defs( file );

		// Background
		file << "  <rect x=\"" << -view_width/2 << "\" y=\"" << -view_height/2 << "\" width=\"" << view_width << "\" height=\"" << view_height << "\" "
		     << SVGWriter::color_attributes( background ) << " />\n";

		// Flip the Y-axis for all visuals
		file << "  <g transform=\"scale(1, -1)\">\n";

		file << body.str();

		file << "  </g>\n";
		file << "</svg>\n";
	}
};
