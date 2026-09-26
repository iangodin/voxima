#include "Producer.h"
#include "FrameStore.h"

#include <SDL3/SDL.h>
#include <lunasvg/lunasvg.h>

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <thread>

namespace
{

// SDL objects, released however preview() is left
struct SdlWindow
{
	SDL_Window *window = nullptr;
	SDL_Renderer *renderer = nullptr;
	SDL_Texture *texture = nullptr;
	SDL_AudioStream *audio = nullptr;

	~SdlWindow()
	{
		if ( audio )
			SDL_DestroyAudioStream( audio );
		if ( texture )
			SDL_DestroyTexture( texture );
		if ( renderer )
			SDL_DestroyRenderer( renderer );
		if ( window )
			SDL_DestroyWindow( window );
		SDL_Quit();
	}
};

std::runtime_error sdl_error( const std::string &what )
{
	return std::runtime_error( what + ": " + SDL_GetError() );
}

// Prefer X11 (through XWayland on a Wayland desktop): the SDL from Conan has no libdecor, so a native Wayland
// window has no border or title bar on GNOME. SDL_VIDEO_DRIVER in the environment still wins. Before SDL_Init.
void prefer_x11()
{
#if defined( __linux__ )
	SDL_SetHint( SDL_HINT_VIDEO_DRIVER, "x11,wayland" );
#endif
}

} // namespace

int Producer::display_frame_rate( size_t audio_rate )
{
	prefer_x11();
	if ( !SDL_Init( SDL_INIT_VIDEO ) )
	{
		throw sdl_error( "SDL_Init" );
	}
	struct Quit
	{
		~Quit()
		{
			SDL_QuitSubSystem( SDL_INIT_VIDEO );
		}
	} quit;

	// The window is not made yet, so it is the main display that counts
	const SDL_DisplayID display = SDL_GetPrimaryDisplay();
	const SDL_DisplayMode *mode = display ? SDL_GetDesktopDisplayMode( display ) : nullptr;
	if ( !mode || mode->refresh_rate < 1.F )
	{
		std::cerr << "Could not find the display's refresh rate, so the frame rate stays at " << _stage.camera.frame_rate
				  << " (--fps sets it)\n";
		return 0;
	}

	// The sound is cut into a whole number of samples for each frame, so the rate must divide the audio rate
	// (144 Hz does not, 128 does): the nearest one at or below the refresh rate
	int rate = static_cast<int>( std::lround( mode->refresh_rate ) );
	while ( rate > 1 && audio_rate % static_cast<size_t>( rate ) != 0 )
	{
		--rate;
	}
	std::cout << "Preview at " << rate << " fps (the display refreshes at " << mode->refresh_rate << " Hz; --fps sets it)\n";
	return rate;
}

bool Producer::preview( const std::string &language )
{
	const std::vector<std::string> languages = { language };
	const std::filesystem::path scratch = std::filesystem::temp_directory_path() / "sonimation-preview";

	// What the player needs of the stage is read here, before the producer thread starts: from then on the
	// thread owns the stage, the script, the screenplay and the narration, and the player touches only the store
	const int width = _stage.camera.width;
	const int height = _stage.camera.height;
	const double fps = static_cast<double>( _stage.camera.frame_rate );
	const int64_t block = static_cast<int64_t>( _stage.audio.audio_block() );
	const int sample_rate = static_cast<int>( _stage.audio.rate() );

	FrameStore store( static_cast<size_t>( block ) );

	// The producer: runs the scenes as fast as it can, and the player takes the frames as it needs them. It
	// stops when asked (on leaving), or when the scenes end, or fail (the player reports it).
	std::jthread producer( [&]( std::stop_token stop )
	{
		try
		{
			_script.select_languages( languages );
			_stage.visuals.clear();
			_stage.audio.begin( scratch, languages, true );
			_current_frame = -1;
			_stage.scene_event = [&]( const std::string &name, bool started )
			{
				if ( started )
					store.scene_started( name, _current_frame );
				else
					store.scene_ended( _current_frame );
			};
			_screenplay = _director( _stage, _script );
			_screenplay.handle.promise().producer_ptr = this;

			while ( !stop.stop_requested() && tick() )
			{
				std::ostringstream svg;
				_stage.save_svg( svg );
				_stage.audio.write_audio();
				store.append( svg.str(), _stage.audio.take_audio( language ) );
			}
			_stage.scene_event = nullptr;
			store.finish();
		}
		catch ( ... )
		{
			_stage.scene_event = nullptr;
			store.fail( std::current_exception() );
		}
	} );

	prefer_x11();
	if ( !SDL_Init( SDL_INIT_VIDEO | SDL_INIT_AUDIO ) )
	{
		throw sdl_error( "SDL_Init" );
	}
	SdlWindow sdl;

	if ( !SDL_CreateWindowAndRenderer( "Sonimation preview", width, height, SDL_WINDOW_RESIZABLE, &sdl.window, &sdl.renderer ) )
	{
		throw sdl_error( "creating the window" );
	}
	SDL_SetRenderLogicalPresentation( sdl.renderer, width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX );
	SDL_SetRenderVSync( sdl.renderer, 1 );

	// lunasvg bitmaps are premultiplied ARGB32, which is what ARGB8888 is in a native-endian 32-bit word
	sdl.texture = SDL_CreateTexture( sdl.renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height );
	if ( !sdl.texture )
	{
		throw sdl_error( "creating the texture" );
	}

	SDL_AudioSpec spec = {};
	spec.format = SDL_AUDIO_S16;
	spec.channels = 2;
	spec.freq = sample_rate;
	sdl.audio = SDL_OpenAudioDeviceStream( SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr );
	if ( !sdl.audio )
	{
		throw sdl_error( "opening the audio device" );
	}

	std::vector<int16_t> chunk; // sound about to be queued

	constexpr int64_t bytes_per_frame = 2 * sizeof( int16_t );
	const int64_t lookahead = std::max<int64_t>( 1, static_cast<int64_t>( 0.25 * fps ) ); // blocks of sound to keep queued

	enum class Mode
	{
		paused,
		forward,
		reverse
	};
	Mode mode = Mode::forward;

	// The sound is the clock. `cursor` is the next frame whose block is to be queued (going forward it is one
	// past the last queued, going backwards one before); the frame being heard is that many blocks back from
	// it as there are blocks still queued. When paused, `playhead` is simply where it is. A cursor beyond the
	// frames made so far waits for the producer, and the picture stays where it is meanwhile.
	int64_t cursor = 0;
	int64_t playhead = 0;
	int64_t pending = -1; // a frame asked for, not made yet
	int64_t shown = -1;
	bool running = false; // the audio device is playing
	bool quit = false;
	int64_t skipped = 0;

	// Repeat: play the scene being watched again and again. Going somewhere with a key (seek) moves the repeat
	// to the scene arrived in; the repeat's own jump back does not.
	bool repeat = false;
	int64_t loop_scene = -1; // which scene (see FrameStore::scene), once known
	bool retarget = false;
	bool jumped = false; // went somewhere on purpose since the last frame shown: what was passed over is not "skipped"

	auto last_frame = [&]() { return static_cast<int64_t>( store.size() ) - 1; };
	auto blocks_queued = [&]()
	{
		const int64_t samples = SDL_GetAudioStreamQueued( sdl.audio ) / bytes_per_frame;
		return ( samples + block - 1 ) / block;
	};

	// A frame's sound put on the end of `to`: played backwards it is its sample frames in reverse order, left and
	// right kept together
	auto add_block = [&]( std::vector<int16_t> &to, const FrameStore::Frame &frame, bool backwards )
	{
		if ( !backwards )
		{
			to.insert( to.end(), frame.audio.begin(), frame.audio.end() );
			return;
		}
		for ( int64_t i = block - 1; i >= 0; --i )
		{
			to.push_back( frame.audio[static_cast<size_t>( 2 * i )] );
			to.push_back( frame.audio[static_cast<size_t>( 2 * i + 1 )] );
		}
	};

	// Stop the sound and go to `frame` (waiting for it, if it has not been made yet)
	auto seek = [&]( int64_t frame )
	{
		retarget = true;
		jumped = true;
		int64_t target = std::max<int64_t>( 0, frame );
		if ( store.complete() )
			target = std::min( target, std::max<int64_t>( 0, last_frame() ) );
		cursor = target;
		if ( target <= last_frame() )
		{
			playhead = target;
			pending = -1;
		}
		else
		{
			pending = target;
		}
		SDL_ClearAudioStream( sdl.audio );
		if ( running )
		{
			SDL_PauseAudioStreamDevice( sdl.audio );
			running = false;
		}
	};
	// Stepping a frame makes a short sound, so it can be heard as well as seen: a frame is too brief to make out
	// (a fortieth of a second), so it is what would be heard playing on from here in that direction, for about
	// a twelfth of a second, faded in and out so it does not click. Not for a frame that is not there yet.
	auto play_snippet = [&]( int64_t frame, bool backwards )
	{
		const int64_t blocks = std::max<int64_t>( 1, static_cast<int64_t>( std::ceil( 0.08 * fps ) ) );
		chunk.clear();
		for ( int64_t i = 0; i < blocks; ++i )
		{
			const auto next = store.frame( backwards ? frame - i : frame + i );
			if ( !next )
				break;
			add_block( chunk, *next, backwards );
		}
		if ( chunk.empty() )
			return;

		const int64_t samples = static_cast<int64_t>( chunk.size() ) / 2;
		const int64_t fade = std::min<int64_t>( samples / 2, sample_rate * 3 / 1000 );
		for ( int64_t i = 0; i < fade; ++i )
		{
			const float gain = static_cast<float>( i ) / static_cast<float>( fade );
			for ( const int64_t at: { i, samples - 1 - i } )
			{
				for ( int64_t channel = 0; channel < 2; ++channel )
				{
					auto &value = chunk[static_cast<size_t>( 2 * at + channel )];
					value = static_cast<int16_t>( static_cast<float>( value ) * gain );
				}
			}
		}

		SDL_PutAudioStreamData( sdl.audio, chunk.data(), static_cast<int>( chunk.size() * sizeof( int16_t ) ) );
		SDL_ResumeAudioStreamDevice( sdl.audio );
		running = true;
	};

	// Pausing stands where the sound has got to; starting (or changing direction) begins again from there
	auto set_mode = [&]( Mode m )
	{
		if ( m != mode || !running )
			seek( playhead );
		mode = m;
	};

	// Where a skip starts from: where we are going, if we are still waiting to get there
	constexpr int64_t the_end = std::numeric_limits<int64_t>::max() / 2; // "the last frame", whenever that is known
	const int64_t skip = static_cast<int64_t>( 1.0 * fps );
	auto skip_by = [&]( int64_t frames ) { seek( std::min( pending >= 0 ? pending : playhead, the_end ) + frames ); };

	// Scenes: [ and ] go to the previous and next start of one (a scene played from inside another starts
	// at a frame of its own, or at the same as the one around it: either way, just frames). A scene the
	// producer has not got to yet is waited for.
	int64_t next_after = -1; // asked for the next scene after this frame, and it has not appeared yet
	auto resolve_next = [&]()
	{
		if ( next_after < 0 )
			return;
		int64_t best = -1;
		for ( const auto &scene: store.scenes() )
		{
			if ( scene.start > next_after && ( best < 0 || scene.start < best ) )
				best = scene.start;
		}
		if ( best >= 0 )
		{
			next_after = -1;
			seek( best );
		}
		else if ( store.complete() )
		{
			next_after = -1;
		}
	};
	auto previous_scene = [&]()
	{
		const int64_t here = pending >= 0 ? pending : playhead;
		int64_t start = 0;  // the start of the scene we are in
		int64_t before = 0; // the start of the one before it
		for ( const auto &scene: store.scenes() )
		{
			if ( scene.start <= here && scene.start > start )
			{
				before = start;
				start = scene.start;
			}
			else if ( scene.start < start && scene.start > before )
			{
				before = scene.start;
			}
		}
		// Like a music player: a little way in, it starts this one again; at the start, it goes to the one before
		seek( here - start > skip ? start : before );
	};

	// The innermost scene that includes `frame`, or -1
	auto scene_index_at = [&]( int64_t frame )
	{
		int64_t found = -1;
		int depth = -1;
		const auto scenes = store.scenes();
		for ( size_t i = 0; i < scenes.size(); ++i )
		{
			if ( scenes[i].start <= frame && ( scenes[i].end < 0 || frame < scenes[i].end ) && scenes[i].depth > depth )
			{
				found = static_cast<int64_t>( i );
				depth = scenes[i].depth;
			}
		}
		return found;
	};

	Uint64 title_time = 0;
	std::string title_text;

	while ( !quit )
	{
		if ( auto error = store.error() )
			std::rethrow_exception( error );

		SDL_Event event;
		while ( SDL_PollEvent( &event ) )
		{
			if ( event.type == SDL_EVENT_QUIT )
			{
				quit = true;
			}
			else if ( event.type == SDL_EVENT_KEY_DOWN )
			{
				switch ( event.key.key )
				{
					case SDLK_ESCAPE: quit = true; break;
					case SDLK_SPACE: set_mode( mode == Mode::paused ? Mode::forward : Mode::paused ); break;
					case SDLK_L: set_mode( Mode::forward ); break;
					case SDLK_J: set_mode( Mode::reverse ); break;
					case SDLK_K: set_mode( Mode::paused ); break;
					case SDLK_LEFT:
						set_mode( Mode::paused );
						seek( playhead - 1 );
						if ( pending < 0 )
							play_snippet( playhead, true );
						break;
					case SDLK_RIGHT:
						set_mode( Mode::paused );
						seek( playhead + 1 );
						if ( pending < 0 )
							play_snippet( playhead, false );
						break;
					case SDLK_HOME:
						set_mode( Mode::paused );
						seek( 0 );
						break;
					case SDLK_END:
						set_mode( Mode::paused );
						seek( the_end );
						break;
					case SDLK_LEFTBRACKET: next_after = -1; previous_scene(); break;
					case SDLK_RIGHTBRACKET:
						next_after = pending >= 0 ? pending : playhead;
						resolve_next();
						break;
					case SDLK_R:
						repeat = !repeat;
						retarget = true;
						break;
					case SDLK_UP: skip_by( skip ); break;
					case SDLK_DOWN: skip_by( -skip ); break;
					default: break;
				}
			}
		}

		resolve_next();

		// A frame that was asked for has been made (or turned out never to be)
		if ( pending >= 0 )
		{
			if ( pending <= last_frame() )
			{
				playhead = pending;
				pending = -1;
			}
			else if ( store.complete() )
			{
				playhead = cursor = std::max<int64_t>( 0, last_frame() );
				pending = -1;
			}
		}

		if ( repeat && retarget && pending < 0 && playhead <= last_frame() )
		{
			loop_scene = scene_index_at( playhead );
			retarget = false;
		}

		if ( mode != Mode::paused )
		{
			// Keep sound queued ahead: what is next in the direction of play (a block played backwards is its
			// sample frames in reverse order, left and right kept together). Nothing is queued for a frame
			// that has not been made yet.
			while ( blocks_queued() < lookahead )
			{
				const auto frame = store.frame( cursor );
				if ( !frame )
					break;

				chunk.clear();
				add_block( chunk, *frame, mode == Mode::reverse );
				SDL_PutAudioStreamData( sdl.audio, chunk.data(), static_cast<int>( block * bytes_per_frame ) );
				cursor += mode == Mode::forward ? 1 : -1;
			}

			const bool at_end = mode == Mode::forward ? ( store.complete() && cursor > last_frame() ) : cursor < 0;
			const int64_t queued = blocks_queued();
			if ( !running && ( queued >= lookahead || ( at_end && queued > 0 ) ) )
			{
				SDL_ResumeAudioStreamDevice( sdl.audio );
				running = true;
			}

			// Where the sound has got to; if that is a frame not made yet, stay where we are
			const int64_t heard = mode == Mode::forward ? cursor - queued : cursor + queued;
			if ( heard < 0 )
				playhead = 0;
			else if ( heard <= last_frame() )
				playhead = heard;

			// Repeating: at the end of the scene (or, playing backwards, its start), go back into it
			bool wrapped = false;
			if ( repeat )
			{
				if ( const auto scene = store.scene( loop_scene ); scene && scene->end >= 0 )
				{
					if ( mode == Mode::forward && heard >= scene->end )
					{
						seek( scene->start );
						wrapped = true;
					}
					else if ( mode == Mode::reverse && heard < scene->start )
					{
						seek( scene->end - 1 );
						wrapped = true;
					}
					retarget = false;
				}
			}

			// Played to the end (or back to the start): stop there
			if ( !wrapped && at_end && queued == 0 )
			{
				set_mode( Mode::paused );
			}
		}

		// Show the frame at the playhead; if the player is slower than the sound, frames in between are skipped
		if ( playhead != shown && playhead <= last_frame() )
		{
			const auto frame = store.frame( playhead );
			auto doc = lunasvg::Document::loadFromData( frame->svg );
			if ( doc == nullptr )
			{
				throw std::runtime_error( "could not load SVG" );
			}
			lunasvg::Bitmap bitmap = doc->renderToBitmap();
			if ( bitmap.isNull() )
			{
				throw std::runtime_error( "could not render to bitmap" );
			}
			SDL_UpdateTexture( sdl.texture, nullptr, bitmap.data(), bitmap.stride() );
			SDL_RenderClear( sdl.renderer );
			SDL_RenderTexture( sdl.renderer, sdl.texture, nullptr, nullptr );
			SDL_RenderPresent( sdl.renderer );

			if ( !jumped && mode != Mode::paused && shown >= 0 && std::abs( playhead - shown ) > 1 )
				skipped += std::abs( playhead - shown ) - 1;
			jumped = false;
			shown = playhead;
		}

		// The window title says where we are, and how far the producer has got
		const Uint64 now = SDL_GetTicks();
		if ( now - title_time >= 100 )
		{
			title_time = now;
			std::ostringstream title;
			// The innermost scene at the frame shown
			std::string scene_name;
			int scene_depth = -1;
			for ( const auto &scene: store.scenes() )
			{
				if ( scene.start <= shown && ( scene.end < 0 || shown < scene.end ) && scene.depth > scene_depth )
				{
					scene_name = scene.name;
					scene_depth = scene.depth;
				}
			}

			title << "Sonimation preview - ";
			if ( !scene_name.empty() )
				title << scene_name << ", ";
			title << std::fixed << std::setprecision( 2 ) << static_cast<double>( shown ) / fps << " s, frame " << shown;
			if ( store.complete() )
				title << " of " << store.size();
			else
				title << " (" << store.size() << " made so far)";
			const bool waiting = pending >= 0 || ( mode != Mode::paused && !store.complete() && cursor > last_frame() );
			if ( repeat )
				title << " - repeating";
			if ( next_after >= 0 )
			{
				title << " - waiting for the next scene";
			}
			else if ( waiting )
			{
				if ( std::max( pending, cursor ) >= the_end )
					title << " - waiting for the end";
				else
					title << " - waiting for frame " << std::max( pending, cursor );
			}
			if ( title.str() != title_text )
			{
				title_text = title.str();
				SDL_SetWindowTitle( sdl.window, title_text.c_str() );
			}
		}

		SDL_Delay( mode == Mode::paused ? 5 : 1 );
	}

	if ( skipped > 0 )
	{
		std::cerr << skipped << " frame(s) were skipped because they were not drawn in time\n";
	}
	return true;
}
