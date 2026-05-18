
#include "Producer.h"

#include "Script.h"

#include <CLI/CLI.hpp>
#include <exporters/FfmpegRender.h>
#include <exporters/FfmpegMux.h>
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

void setup_codecs( std::string_view container, std::string &vcodec, std::string &acodec )
{
	if ( container == "mp4" )
	{
		if ( vcodec.empty() )
		{
			vcodec = "h264";
		}
		if ( acodec.empty() )
		{
			acodec = "aac";
		}

		if ( vcodec == "h264" || vcodec == "h265" || vcodec == "vp9" || vcodec == "av1" )
		{
			// allowed
		}
		else
		{
			throw std::runtime_error( "unknown video codec " + std::string( vcodec ) );
		}

		if ( acodec == "aac" || acodec == "opus" )
		{
			// allowed
		}
		else if ( acodec == "wav" || acodec == "flac" || acodec == "vorbis" )
		{
			throw std::runtime_error( std::string( acodec ) + " is not supported in mp4 container" );
		}
		else
		{
			throw std::runtime_error( "unknown audio codec " + std::string( acodec ) );
		}
	}
	else if ( container == "mkv" )
	{
		if ( vcodec.empty() )
		{
			vcodec = "h264";
		}
		if ( acodec.empty() )
		{
			acodec = "aac";
		}

		if ( vcodec == "h264" || vcodec == "h265" || vcodec == "vp9" || vcodec == "av1" )
		{
			// allowed
		}
		else
		{
			throw std::runtime_error( "unknown video codec " + std::string( vcodec ) );
		}

		if ( acodec == "aac" || acodec == "opus" || acodec == "wav" || acodec == "flac" || acodec == "vorbis" )
		{
			// allowed
		}
		else
		{
			throw std::runtime_error( "unknown audio codec " + std::string( acodec ) );
		}
	}
	else if ( container == "webm" )
	{
		if ( vcodec.empty() )
		{
			vcodec = "av1";
		}
		if ( acodec.empty() )
		{
			acodec = "opus";
		}

		if ( vcodec == "vp9" || vcodec == "av1" )
		{
			// allowed
		}
		else if ( vcodec == "h264" || vcodec == "h265" )
		{
			throw std::runtime_error( std::string( vcodec ) + " is not supported in webm container" );
		}
		else
		{
			throw std::runtime_error( "unknown video codec " + std::string( vcodec ) );
		}

		if ( acodec == "vorbis" || acodec == "opus" )
		{
			// allowed
		}
		else if ( acodec == "wav" || acodec == "flac" || acodec == "aac" )
		{
			throw std::runtime_error( std::string( acodec ) + " is not supported in mp4 container" );
		}
		else
		{
			throw std::runtime_error( "unknown audio codec " + std::string( acodec ) );
		}
	}
	else
	{
		throw std::runtime_error( "unknown container " + std::string( container ) );
	}
}

bool Producer::produce( int argc, char *argv[] )
{
	CLI::App app{ "Sonimation" };
	argv = app.ensure_utf8( argv );

	std::string container = "mkv";
	app.add_option( "-c,--container", container, "Container (mp4, mkv, webm)" );

	std::string vcodec;
	app.add_option( "-v,--vcodec", vcodec, "Video codec (h264, h265, vp9 or av1)" );

	std::string acodec;
	app.add_option( "-a,--acodec", acodec, "Video codec (flac, wav, aac, opus)" );

	std::optional<std::string> font_path;
	app.add_option( "--font-dir", font_path, "Directory containing fonts" );

	std::string render_dir = "render";
	app.add_option( "--render-dir", render_dir, "Directory for render to" );

	std::string script_path;
	app.add_option( "--script-dir", script_path, "Directory containing scripts" );

	std::optional<std::string> theme_name;
	app.add_option( "--theme", theme_name, "Theme to use" );

	app.add_option( "--width", _stage.camera.width, "Camera width" );
	app.add_option( "--height", _stage.camera.height, "Camera height" );
	app.add_option( "--fps", _stage.camera.frame_rate, "Camera frame rate" );

	std::vector<std::string> languages;
	app.add_option( "--lang", languages, "Language to use (first one is master, multiple allowed)" );

	CLI11_PARSE( app, argc, argv );

	setup_codecs( container, vcodec, acodec );

	std::cout << "Encoding " << vcodec << "+" << acodec << " into " << container << '\n';

	if ( script_path.empty() )
	{
		throw std::runtime_error( "script path is required" );
	}
	_script.set_directory( script_path );

	if ( languages.empty() )
	{
		throw std::runtime_error( "at least one language is required" );
	}
	for ( auto &lang: languages )
	{
		if ( !_script.load( lang ) )
		{
			throw std::runtime_error( "unable to load script for " + lang );
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

	/*
	if ( !AudioMixer::supported_codec( acodec ) )
	{
		throw std::runtime_error( "unsupported audio codec " + acodec );
	}
	*/

	_stage.audio.setup( render_dir, "flac", 48000, static_cast<size_t>( _stage.camera.frame_rate ) );
	for ( auto &lang: languages )
	{
		_stage.audio.add_language( lang );
	}

	_screenplay = _director( _stage, _script );
	_screenplay.handle.promise().producer_ptr = this;

	std::optional<FfmpegRender> ffmpeg;
	std::optional<PngRender> pngs;
	std::tuple<std::string, int, std::string> svgs = {};
	bool write_svgs = false;

	const std::string video_filename = render_dir + "/video.mkv";
	ffmpeg = FfmpegRender( video_filename, vcodec, static_cast<double>( _stage.camera.frame_rate ) );

	const std::string final_filename = render_dir + "/final." + container;
	/*
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
	*/

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
			_stage.audio.write_audio();
		}
		++frame;
	}
	timer_total.message( "Total render " + std::to_string( frame ) + " frames: ", frame );

	if ( ffmpeg )
	{
		ffmpeg->close();
	}
	_stage.audio.close();

	if ( !video_filename.empty() && !final_filename.empty() )
	{
		Timer timer_mux( "  FFMPEG muxing: " );
		FfmpegMux mux( final_filename, acodec );
		mux.add_video_track( video_filename );
		for ( auto &lang: languages )
		{
			mux.add_audio_track( _stage.audio.output( lang ), lang, {} );
		}
		mux.mux();
	}

	return 0;
}
