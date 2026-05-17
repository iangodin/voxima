
#pragma once

#include <visuals/Group.h>
#include <visuals/Color.h>

#include <audio/DubbingStage.h>

#include <exporters/SVGWriter.h>

#include <core/Theme.h>
#include <core/Camera.h>

#include <iostream>
#include <sstream>

struct Stage
{
	Theme *theme = nullptr;
	Color background = { 0.F, 0.F, 0.F, 1.F };
	Group visuals;
	DubbingStage audio;
	Camera camera;

	void save_svg( std::ostream &file ) const
	{
		if ( !file ) return;

		std::ostringstream body;
		SVGWriter writer( body );
		visuals.accept( writer );

		float aspect = static_cast<float>( camera.height ) / static_cast<float>( camera.width );
		float view_width = 16.f;
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
