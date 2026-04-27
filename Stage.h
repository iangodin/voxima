
#pragma once

#include "Visuals.h"
#include "SVGWriter.h"
#include "Color.h"

#include <string>
#include <fstream>
#include <vector>
#include <iostream>

struct Stage
{
	Color background = { 0.F, 0.F, 0.F, 1.F };
	Visuals visuals;

	void save_svg( const std::string &filename ) const
	{
		std::ofstream file( filename );
		if ( !file ) return;

		file << "<svg width=\"1280\" height=\"720\" "
		     << "viewBox=\"-8 -4.5 16 9\" "
		     << "xmlns=\"http://www.w3.org/2000/svg\">\n";

		// Background
		file << "  <rect x=\"-8\" y=\"-4.5\" width=\"16\" height=\"9\" "
		     << SVGWriter::color_attributes( background ) << " />\n";

		// Flip the Y-axis for all visuals
		file << "  <g transform=\"scale(1, -1)\">\n";

		SVGWriter writer( file );
		visuals.accept( writer );

		file << "  </g>\n";
		file << "</svg>\n";
	}
};
