
#include "Producer.h"
#include "SceneRegistry.h"

#include <visuals/MissingGlyph.h>

#include "Script.h"

#include <CLI/CLI.hpp>
#include <exporters/FfmpegRender.h>
#include <exporters/FfmpegMux.h>
#include <exporters/PngRender.h>
#include <exporters/SubtitleWriter.h>
#include <optional>
#include <core/ThemeFile.h>
#include <utils/Language.h>
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

// scodec/subtitle_ext pick the subtitle codec and sidecar file extension to
// feed ffmpeg per container: mp4 only broadly supports mov_text (fed from an
// .srt input, transcoded), mkv natively supports .srt, webm only supports
// WebVTT (.vtt) - it doesn't accept SRT at all.
void setup_codecs(
	std::string_view container,
	std::string &vcodec,
	std::string &acodec,
	std::string &scodec,
	std::string &subtitle_ext )
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

		scodec = "mov_text";
		subtitle_ext = "srt";
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

		scodec = "srt";
		subtitle_ext = "srt";
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

		scodec = "webvtt";
		subtitle_ext = "vtt";
	}
	else
	{
		throw std::runtime_error( "unknown container " + std::string( container ) );
	}
}

bool Producer::render( const RenderJob &job )
{
	// A fresh start: the tracks for these languages, the languages in use, an empty stage
	_script.select_languages( job.languages );
	_stage.visuals.clear();
	_stage.audio.begin( job.render_dir, job.languages );
	_current_frame = -1;

	_screenplay = _director( _stage, _script );
	_screenplay.handle.promise().producer_ptr = this;

	std::optional<FfmpegRender> ffmpeg;
	std::optional<PngRender> pngs;
	std::tuple<std::string, int, std::string> svgs = {};
	bool write_svgs = false;

	const std::string video_filename = job.render_dir + "/video.mkv";
	ffmpeg = FfmpegRender( video_filename, job.vcodec, static_cast<double>( _stage.camera.frame_rate ) );

	const std::string final_filename = job.render_dir + "/final." + job.container;

	if ( job.save_png )
	{
		pngs = PngRender( job.render_dir + "/frame.", ".png", 8 );
	}

	if ( job.save_svg )
	{
		write_svgs = true;
		svgs = { job.render_dir + "/frame.", 8, ".svg" };
	}

	size_t frame = 0;
	Timer timer_total;
	while ( true )
	{
		Timer timer_frame( "Frame " + std::to_string( frame ) + ": " );
		{
			//Timer timer_tick( "  tick: " );
			if ( !tick() )
			{
				//timer_tick.abort();
				timer_frame.abort();
				break;
			}
		}

		std::ostringstream svg;
		{
			//Timer timer_svg_gen( "  SVG generation: " );
			_stage.save_svg( svg );
		}

		if ( ffmpeg || pngs || write_svgs )
		{
			if ( write_svgs )
			{
				//Timer timer_svg_save( "  SVG saving: " );
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
					//Timer timer_render( "  SVG rendering: " );
					bitmap = doc->renderToBitmap();
					if ( bitmap.isNull() )
					{
						std::cerr << "ERROR: could not render to bitmap\n";
						return false;
					}
				}

				if ( ffmpeg )
				{
					//Timer timer_ffmpeg( "  FFMPEG encoding: " );
					ffmpeg->encode( bitmap );
				}

				if ( pngs )
				{
					//Timer timer_pngs( "  PNG compression: " );
					pngs->encode( bitmap );
				}
			}
		}

		{
			//Timer timer_audio_enc( "  Audio encoding: " );
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

		bool any_subtitles = false;
		for ( const auto &lang: job.languages )
		{
			if ( !_stage.audio.subtitles( lang ).empty() )
			{
				any_subtitles = true;
				break;
			}
		}

		FfmpegMux mux( final_filename, job.acodec, any_subtitles ? job.scodec : std::string{} );
		mux.add_video_track( video_filename );
		for ( const auto &lang: job.languages )
		{
			// Container "language" metadata is conventionally a bare ISO 639
			// code, not a full BCP-47 regional tag - "en-GB" isn't itself a
			// valid value the way "en" is.
			mux.add_audio_track(
				_stage.audio.output( lang ), language::primary_subtag( lang ), {},
				lang == job.languages.front() );
		}
		for ( const auto &lang: job.languages )
		{
			const auto &cues = _stage.audio.subtitles( lang );
			if ( cues.empty() )
			{
				continue;
			}

			const std::string srt_path = job.render_dir + "/subtitles." + lang + ".srt";
			const std::string vtt_path = job.render_dir + "/subtitles." + lang + ".vtt";
			write_srt( srt_path, cues );
			write_vtt( vtt_path, cues );

			mux.add_subtitle_track(
				job.subtitle_ext == "vtt" ? vtt_path : srt_path, language::primary_subtag( lang ), {},
				lang == job.languages.front() );
		}
		mux.mux();
	}

	return true;
}

namespace
{
std::string palette_names()
{
	std::string out;
	for ( const std::string &name: Palette::names() )
	{
		out += ( out.empty() ? "" : ", " ) + name;
	}
	return out;
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

	bool save_svg = false;
	app.add_flag( "--save-svg", save_svg, "Save SVG sequence" );

	bool save_png = false;
	app.add_flag( "--save-png", save_png, "Save PNG sequence" );

	bool live_preview = false;
	app.add_flag( "--preview", live_preview, "Play in a window with sound instead of rendering a video (first language only)" );

	std::string script_path;
	app.add_option( "--script-dir", script_path, "Directory containing scripts" );

	std::optional<std::string> theme_name;
	app.add_option( "--theme", theme_name, "Theme to use: a built-in one by name, or a theme file (a path, or <name>.huml in a --theme-dir). Without it, theme.huml in the script directory, if there is one" );

	std::vector<std::string> theme_dirs;
	app.add_option( "--theme-dir", theme_dirs, "Directory to look in for theme files, and for the themes they inherit (repeatable; the script directory is always searched)" );

	std::optional<std::string> palette_name;
	app.add_option(
		"--palette",
		palette_name,
		"Colors to use, apart from the theme's fonts: " + palette_names() + ". Without it, the theme's own on the usual black" );

	app.add_option( "--width", _stage.camera.width, "Camera width" );
	app.add_option( "--height", _stage.camera.height, "Camera height" );
	auto *fps_option = app.add_option(
		"--fps", _stage.camera.frame_rate, "Camera frame rate (24 by default; with --preview, the display's refresh rate)" );

	std::vector<std::string> languages;
	app.add_option( "--lang", languages, "Language to use (first one is master, multiple allowed)" );

	std::string tts_model = "models/Chatterbox-GGUF/chatterbox-q8_0.gguf";
	app.add_option( "--tts-model", tts_model, "Path to the Chatterbox GGUF model" );

	std::string tts_voice_dir = "voices";
	app.add_option( "--tts-voice-dir", tts_voice_dir, "Base directory for narrator reference-clip paths" );

	std::string tts_backend = "best";
	app.add_option( "--tts-backend", tts_backend, "TTS backend: best, cpu or cuda" );

	int tts_threads = 4;
	app.add_option( "--tts-threads", tts_threads, "TTS CPU thread count" );

	std::string aligner_model = "models/Qwen3-ForcedAligner-0.6B-GGUF/qwen3-forced-aligner-0.6b-q8_0.gguf";
	app.add_option(
		"--aligner-model", aligner_model,
		"Path to the Qwen3 Forced Aligner GGUF model, for subtitle timing (optional)" );

	std::vector<std::string> scene_names;
	app.add_option( "--scene", scene_names, "Scene to render; repeat it to play several in order (default: \"movie\")" );

	double max_speedup = 1.0;
	app.add_option(
		"--max-speedup",
		max_speedup,
		"How much the other languages may be speeded up to keep in step with the first, in one video (1.0: not at all, they are padded with silence; 1.15: up to 15%; needs ffmpeg)" );

	bool separate = false;
	app.add_flag(
		"--separate",
		separate,
		"One video per language, each timed to its own narration, in <render-dir>/<lang>/ (instead of one video with a track for each language)" );

	bool strict_glyphs = false;
	app.add_flag( "--strict-glyphs", strict_glyphs, "Exit with an error if any text had a missing glyph (they always show as magenta boxes and are listed at the end)" );

	bool list_scenes = false;
	app.add_flag( "--list-scenes", list_scenes, "List the scenes that can be rendered, and exit" );

	CLI11_PARSE( app, argc, argv );

	if ( list_scenes )
	{
		std::cout << "Scenes:\n" << describe_scenes();
		return true;
	}

	clear_missing_glyph_reports();

	// Decide what to render before anything slow (models, fonts) loads
	if ( !scene_names.empty() )
	{
		const auto unknown = unknown_scenes( scene_names );
		if ( !unknown.empty() )
		{
			std::string message = "unknown scene:";
			for ( const auto &name: unknown )
			{
				message += " " + name;
			}
			throw std::runtime_error( message + "\nScenes:\n" + describe_scenes() );
		}
		_director = [scene_names]( Stage &stage, Script &script ) { return sequence( stage, script, scene_names ); };
	}
	else if ( !_director )
	{
		if ( SceneEntry::get_by_name( "movie" ) )
		{
			_director = []( Stage &stage, Script &script ) { return play( stage, script, "movie" ); };
		}
		else
		{
			const auto scenes = SceneEntry::all();
			if ( scenes.size() != 1 )
			{
				throw std::runtime_error( "no scene to render: give --scene <name>, or register one called \"movie\"\nScenes:\n" + describe_scenes() );
			}
			const std::string only = scenes.front()->get_name();
			_director = [only]( Stage &stage, Script &script ) { return play( stage, script, only ); };
		}
	}

	if ( max_speedup < 1.0 )
	{
		throw std::runtime_error( "--max-speedup must be at least 1.0 (1.0: no speeding up)" );
	}

	std::string scodec;
	std::string subtitle_ext;
	setup_codecs( container, vcodec, acodec, scodec, subtitle_ext );

	std::cout << "Encoding " << vcodec << "+" << acodec << " into " << container << '\n';

	if ( script_path.empty() )
	{
		throw std::runtime_error( "script_path is required" );
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

	// The theme: a built-in one, or one from a file (see ThemeFile.h): by name in a theme directory, or a path; without --theme,
	// theme.huml next to the scripts
	std::vector<std::filesystem::path> theme_directories( theme_dirs.begin(), theme_dirs.end() );
	theme_directories.push_back( script_path );
	if ( theme_name )
	{
		_stage.theme = Theme::get_by_name( *theme_name );
		if ( _stage.theme == nullptr )
		{
			_loaded_theme.emplace( themes::load( *theme_name, theme_directories ) );
			_stage.theme = &*_loaded_theme;
		}
	}
	else if ( const auto file = std::filesystem::path( script_path ) / "theme.huml"; std::filesystem::exists( file ) )
	{
		_loaded_theme.emplace( themes::load( file.string(), theme_directories ) );
		_stage.theme = &*_loaded_theme;
	}

	if ( _stage.theme == nullptr )
	{
		std::string known;
		for ( const auto &theme: Theme::get_registry() )
		{
			known += "\n  " + theme.first;
		}
		throw std::runtime_error( "no theme: give --theme with a built-in one's name, or a theme file (or put theme.huml with the scripts). The themes:" + known );
	}

	if ( font_path )
	{
		_stage.theme->set_font_path( *font_path );
	}

	// A palette of its own choosing: its background and its text color for the picture (light or dark), and its colors
	// for code, plots and what stands out. Otherwise the theme's palette gives only those last, on black as always.
	if ( palette_name )
	{
		_stage.theme->set_palette( Palette::get( *palette_name ) );
		_stage.use_palette( _stage.theme->palette() );
	}

	constexpr size_t audio_rate = 48000;

	// A preview goes at the speed of the screen it is on, unless told otherwise: the scenes are written in
	// seconds, so they last as long at any rate
	if ( live_preview && fps_option->count() == 0 )
	{
		if ( const int rate = display_frame_rate( audio_rate ); rate > 0 )
		{
			_stage.camera.frame_rate = rate;
		}
	}

	_stage.audio.setup(
		render_dir,
		"flac",
		audio_rate,
		static_cast<size_t>( _stage.camera.frame_rate ),
		tts_model,
		tts_voice_dir,
		tts_backend,
		tts_threads,
		aligner_model );
	_stage.audio.set_max_speedup( max_speedup );

	// Text that could not be drawn was shown as magenta boxes; say what and where, so it can be fixed. This does not depend
	// on seeing the boxes: they cannot be told apart from a background or a text of the same color, and a preview has no
	// end-of-render to look at.
	auto report_missing_glyphs = [&]()
	{
		const std::string missing = describe_missing_glyphs();
		if ( missing.empty() )
		{
			return true;
		}
		std::cerr << '\n' << missing;
		if ( strict_glyphs )
		{
			std::cerr << "Failing because of --strict-glyphs.\n";
			return false;
		}
		return true;
	};

	if ( live_preview )
	{
		const bool ok = preview( languages.front() );
		return report_missing_glyphs() && ok;
	}

	// A render for all the languages together (one container with a track for each), or with --separate one
	// for each language, timed to that language's own narration
	bool ok = true;
	if ( separate )
	{
		for ( const auto &lang: languages )
		{
			ok = render( { { lang }, render_dir + "/" + lang, container, vcodec, acodec, scodec, subtitle_ext, save_png, save_svg } ) && ok;
		}
	}
	else
	{
		ok = render( { languages, render_dir, container, vcodec, acodec, scodec, subtitle_ext, save_png, save_svg } );
	}

	if ( !report_missing_glyphs() )
	{
		return false;
	}

	return ok;
}
