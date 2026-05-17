
#include "Producer.h"

#include "Script.h"

#include <CLI/CLI.hpp>
#include <exporters/FfmpegRender.h>
#include <exporters/PngRender.h>
#include <optional>
#include <utils/ParseSequenceName.h>
#include <utils/Timer.h>

std::string read_file( std::filesystem::path path )
{
	std::size_t size = std::filesystem::file_size( path );
	std::ifstream file( path, std::ios::binary );
	std::string buffer( size, '\0' );
	file.read( buffer.data(), std::streamsize( size ) );
	return buffer;
}

bool Producer::produce( int argc, char *argv[] )
{
	CLI::App app{ "Sonimation" };
	argv = app.ensure_utf8( argv );

	std::vector<std::string> outputs;
	app.add_option( "-o,--output", outputs, "Output file (mp4, wav, flac, png, or svg)" );

	std::string vcodec;
	app.add_option( "--vcodec", vcodec, "Video codec (h264 or h265)" );

	std::optional<std::string> font_path;
	app.add_option( "--fonts", font_path, "Directory containing fonts" );

	std::optional<std::string> theme_name;
	app.add_option( "--theme", theme_name, "Theme to use" );

	app.add_option( "--tts-cache", _stage.tts_cache, "Directory for TTS cache" );
	app.add_option( "--width", _stage.camera.width, "Camera width" );
	app.add_option( "--height", _stage.camera.height, "Camera height" );
	app.add_option( "--fps", _stage.camera.frame_rate, "Camera frame rate" );

	std::optional<std::string> script_path;
	app.add_option( "--scripts", script_path, "Directory containing scripts" );

	std::optional<std::string> language;
	app.add_option( "--lang", language, "Language to use" );

	CLI11_PARSE( app, argc, argv );

	if ( language )
	{
		_script.set_directory( script_path.value_or( "." ) );
		if ( !_script.load( *language ) )
		{
			throw std::runtime_error( "unable to load script " + *language );
		}
		else
		{
			std::cout << "Script:\n" << _script << '\n';
		}
	}

	if ( theme_name )
	{
		_stage.theme = Theme::get_by_name( *theme_name );
	}

	if ( _stage.theme == nullptr )
	{
		std::cout << "Available themes:\n";
		for ( const auto &theme: Theme::get_registry() )
		{
			std::cout << "  " << theme.first << '\n';
		}
		throw std::runtime_error( "missing theme: " + *theme_name );
	}

	if ( font_path )
	{
		_stage.theme->set_font_path( *font_path );
	}

	_screenplay = std::move( _director( _stage, _script ) );
	_screenplay.handle.promise().producer_ptr = this;

	std::optional<FfmpegRender> ffmpeg;
	std::string video_filename;
	std::string audio_filename;
	std::string final_filename;
	std::optional<PngRender> pngs;
	std::tuple<std::string, int, std::string> svgs = {};
	bool write_svgs = false;

	for ( const auto &output: outputs )
	{
		if ( output.ends_with( ".mp4" ) )
		{
			if ( !final_filename.empty() )
			{
				throw std::runtime_error( "only one final mp4 allowed" );
			}
			final_filename = output;
		}
		else if ( output.ends_with( ".ts" )  )
		{
			if ( ffmpeg.has_value() )
			{
				throw std::runtime_error( "only one output video allowed" );
			}
			video_filename = output;
			ffmpeg = FfmpegRender( output, vcodec, static_cast<double>( _stage.camera.frame_rate ) );
		}
		else if ( AudioMixer::supported_format( output ) )
		{
			if ( !_stage.audio.open( output, 48000 ) )
			{
				throw std::runtime_error( "could not open audio output: " + output );
			}
			audio_filename = output;
		}
		else if ( output.ends_with( ".png" ) )
		{
			if ( pngs.has_value() )
			{
				throw std::runtime_error( "only one output png sequence allowed" );
			}
			auto filename = parse_sequence_name( output );
			pngs = PngRender(
				std::get<0>( filename ), std::get<2>( filename ), std::get<1>( filename ) );
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
		else
		{
			throw std::runtime_error( "unknown output " + output );
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
				filename << std::get<0>( svgs ) << std::setw( std::get<1>( svgs ) )
						 << std::setfill( '0' ) << frame << std::get<2>( svgs );
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

		{
			Timer timer_svg_gen( "  Audio encoding: " );
			_stage.audio.save_audio();
		}
		++frame;
	}
	timer_total.message( "Total render " + std::to_string( frame ) + " frames: ", frame );

	if ( ffmpeg )
	{
		ffmpeg->close();
	}
	_stage.audio.close();

	if ( !video_filename.empty() && !audio_filename.empty() && !final_filename.empty() )
	{
		Timer timer_mux( "  FFMPEG muxing: " );
		double framerate = static_cast<double>( _stage.camera.frame_rate );
		FfmpegRender::mux( framerate, video_filename, audio_filename, final_filename );
	}

	return 0;
}
