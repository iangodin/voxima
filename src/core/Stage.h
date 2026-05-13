
#pragma once

#include <visuals/Group.h>
#include <visuals/Color.h>

#include <audio/AudioMixer.h>

#include <exporters/SVGWriter.h>

#include <core/Theme.h>
#include <core/Camera.h>

#include <tts/asio_wrapper.h>

#include <iostream>
#include <sstream>
#include <optional>
#include <filesystem>

struct Stage
{
	Theme *theme = nullptr;
	Color background = { 0.F, 0.F, 0.F, 1.F };
	Group visuals;
	AudioMixer audio;
	Camera camera;
	asio::io_context io_context;
	std::optional<std::filesystem::path> tts_cache;

	void save_svg( std::ostream &file ) const
	{
		if ( !file ) return;

		std::ostringstream body;
		SVGWriter writer( body );
		visuals.accept( writer );

		file << "<svg width=\"1280\" height=\"720\" "
		     << "viewBox=\"-8 -4.5 16 9\" "
		     << "xmlns=\"http://www.w3.org/2000/svg\">\n";

		writer.write_defs( file );

		// Background
		file << "  <rect x=\"-8\" y=\"-4.5\" width=\"16\" height=\"9\" "
		     << SVGWriter::color_attributes( background ) << " />\n";

		// Flip the Y-axis for all visuals
		file << "  <g transform=\"scale(1, -1)\">\n";

		file << body.str();

		file << "  </g>\n";
		file << "</svg>\n";
	}
};
