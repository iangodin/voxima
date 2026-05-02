
#include "Producer.h"

#include <exporters/FfmpegRender.h>
#include <exporters/PngRender.h>

#include <utils/Timer.h>
#include <utils/ParseSequenceName.h>

#include <CLI/CLI.hpp>

#include <optional>
#include <stdexcept>

bool Producer::produce( int argc, char *argv[] )
{
	CLI::App app{ "Sonimation" };
	argv = app.ensure_utf8( argv );

	std::vector<std::string> outputs;
	app.add_option( "-o,--output", outputs, "Output file (mp4, png, or svg)" );

	std::optional<std::string> font_path;
	app.add_option( "--fonts", font_path, "Directory containing fonts" );

	CLI11_PARSE( app, argc, argv );

	if ( font_path )
	{
		_stage.theme.set_font_path( *font_path );
	}

	std::optional<FfmpegRender> ffmpeg;
	std::optional<PngRender> pngs;
	std::tuple<std::string,int,std::string> svgs = {};
	bool write_svgs = false;

	for ( const auto &output: outputs )
	{
		if ( output.ends_with( ".mp4" ) )
		{
			if ( ffmpeg.has_value() )
			{
				throw std::runtime_error( "only one output mp4 allowed" );
			}
			ffmpeg = FfmpegRender( output );
		}
		else if ( output.ends_with( ".png" ) )
		{
			if ( pngs.has_value() )
			{
				throw std::runtime_error( "only one output png sequence allowed" );
			}
			auto filename = parse_sequence_name( output );
			pngs = PngRender( std::get<0>( filename ), std::get<2>( filename ), std::get<1>( filename ) );
		}
		else if ( output.ends_with( ".svg" ) )
		{
			if ( write_svgs )
			{
				throw std::runtime_error( "only one output svg sequence allowed" );
			}
			write_svgs = true;
			svgs = parse_sequence_name( output );
		}
	}

	size_t frame = 0;
	Timer timer_total;
	while ( true )
	{
		Timer timer_frame( "Frame " + std::to_string( frame ) + ": " );
		{
			Timer timer_tick( "  tick: " );
			if ( !tick() )
			{
				timer_tick.abort();
				timer_frame.abort();
				break;
			}
		}

		std::ostringstream svg;
		{
			Timer timer_svg_gen( "  SVG generation: " );
			_stage.save_svg( svg );
		}

		if ( ffmpeg || pngs || write_svgs )
		{
			if ( write_svgs )
			{
				Timer timer_svg_save( "  SVG saving: " );
				std::stringstream filename;
				filename << std::get<0>( svgs ) << std::setw( std::get<1>( svgs ) ) << std::setfill( '0' ) << frame << std::get<2>( svgs );
				std::ofstream svg_file( filename.str() );
				svg_file << svg.str();
			}

			if ( ffmpeg || pngs )
			{
				auto doc = lunasvg::Document::loadFromData( svg.str() );
				if ( doc == nullptr )
				{
					throw std::runtime_error( "could not load SVG" );
				}

				lunasvg::Bitmap bitmap;
				{
					Timer timer_render( "  SVG rendering: " );
					bitmap = doc->renderToBitmap();
					if ( bitmap.isNull() )
					{
						std::cerr << "ERROR: could not render to bitmap\n";
						return -1;
					}
				}

				if ( ffmpeg )
				{
					Timer timer_ffmpeg( "  FFMPEG encoding: " );
					ffmpeg->encode( bitmap );
				}

				if ( pngs )
				{
					Timer timer_pngs( "  PNG comression: " );
					pngs->encode( bitmap );
				}
			}
		}

		++frame;
	}
	timer_total.message( "Total render " + std::to_string( frame ) + " frames: ", frame );

	return 0;
}


