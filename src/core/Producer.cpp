
#include "Producer.h"

#include "Script.h"

#include <exporters/FfmpegRender.h>
#include <exporters/PngRender.h>

#include <utils/Timer.h>
#include <utils/ParseSequenceName.h>

#include <CLI/CLI.hpp>

#include <optional>

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
	app.add_option( "-o,--output", outputs, "Output file (mp4, wav, png, or svg)" );

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
		std::filesystem::path script_file = script_path.value_or( "." );
		script_file /= language.value_or( "en" ) + ".huml";

		std::string source = read_file( script_file );
		if ( !_script.parse( source ) )
		{
			throw std::runtime_error( "unable to parse script " + script_file.string() );
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
	std::string ffmpeg_filename;
	std::string audio_filename;
	std::optional<PngRender> pngs;
	std::tuple<std::string, int, std::string> svgs = {};
	bool write_svgs = false;

	for ( const auto &output: outputs )
	{
		if ( output.ends_with( ".mp4" ) )
		{
			if ( ffmpeg.has_value() )
			{
				throw std::runtime_error( "only one output mp4 allowed" );
			}
			ffmpeg_filename = output;
			ffmpeg = FfmpegRender( output, static_cast<double>(_stage.camera.frame_rate) );
		}
		else if ( output.ends_with( ".wav" ) )
		{
			if ( !_stage.audio.open( output, 48000 ) )
			{
				throw std::runtime_error( "could not open audio output: " + output );
			}
			audio_filename = output;
		}		else if ( output.ends_with( ".png" ) )
		{
			if ( pngs.has_value() )
			{
				throw std::runtime_error( "only one output png sequence allowed" );
			}
			auto filename = parse_sequence_name( output );
			pngs = PngRender(
				std::get<0>( filename ),
				std::get<2>( filename ),
				std::get<1>( filename ) );
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

		_stage.audio.save_audio();
		++frame;
	}
	timer_total.message( "Total render " + std::to_string( frame ) + " frames: ", frame );

	if ( ffmpeg )
	{
		ffmpeg->close();
	}
	_stage.audio.close();

	if ( !ffmpeg_filename.empty() && !audio_filename.empty() )
	{
		Timer timer_mux( "  FFMPEG muxing: " );
		std::string temp_video = ffmpeg_filename + ".tmp.mp4";
		std::filesystem::rename( ffmpeg_filename, temp_video );
		FfmpegRender::mux( temp_video, audio_filename, ffmpeg_filename );
		std::filesystem::remove( temp_video );
	}

	return 0;
}

