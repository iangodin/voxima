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

} // namespace

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
			_screenplay = _director( _stage, _script );
			_screenplay.handle.promise().producer_ptr = this;

			while ( !stop.stop_requested() && tick() )
			{
				std::ostringstream svg;
				_stage.save_svg( svg );
				_stage.audio.write_audio();
				store.append( svg.str(), _stage.audio.take_audio( language ) );
			}
			store.finish();
		}
		catch ( ... )
		{
			store.fail( std::current_exception() );
		}
	} );

#if defined( __linux__ )
	// Prefer X11 (through XWayland on a Wayland desktop): the SDL from Conan has no libdecor, so a native
	// Wayland window has no border or title bar on GNOME. SDL_VIDEO_DRIVER in the environment still wins.
	SDL_SetHint( SDL_HINT_VIDEO_DRIVER, "x11,wayland" );
#endif
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

	std::vector<int16_t> reversed( static_cast<size_t>( block ) * 2 );

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

	auto last_frame = [&]() { return static_cast<int64_t>( store.size() ) - 1; };
	auto blocks_queued = [&]()
	{
		const int64_t samples = SDL_GetAudioStreamQueued( sdl.audio ) / bytes_per_frame;
		return ( samples + block - 1 ) / block;
	};

	// Stop the sound and go to `frame` (waiting for it, if it has not been made yet)
	auto seek = [&]( int64_t frame )
	{
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
						break;
					case SDLK_RIGHT:
						set_mode( Mode::paused );
						seek( playhead + 1 );
						break;
					case SDLK_HOME:
						set_mode( Mode::paused );
						seek( 0 );
						break;
					case SDLK_END:
						set_mode( Mode::paused );
						seek( the_end );
						break;
					case SDLK_UP: skip_by( skip ); break;
					case SDLK_DOWN: skip_by( -skip ); break;
					default: break;
				}
			}
		}

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

				if ( mode == Mode::forward )
				{
					SDL_PutAudioStreamData( sdl.audio, frame->audio.data(), static_cast<int>( block * bytes_per_frame ) );
					++cursor;
				}
				else
				{
					for ( int64_t i = 0; i < block; ++i )
					{
						reversed[static_cast<size_t>( 2 * i )] = frame->audio[static_cast<size_t>( 2 * ( block - 1 - i ) )];
						reversed[static_cast<size_t>( 2 * i + 1 )] = frame->audio[static_cast<size_t>( 2 * ( block - 1 - i ) + 1 )];
					}
					SDL_PutAudioStreamData( sdl.audio, reversed.data(), static_cast<int>( block * bytes_per_frame ) );
					--cursor;
				}
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

			// Played to the end (or back to the start): stop there
			if ( at_end && queued == 0 )
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

			if ( mode != Mode::paused && shown >= 0 && std::abs( playhead - shown ) > 1 )
				skipped += std::abs( playhead - shown ) - 1;
			shown = playhead;
		}

		// The window title says where we are, and how far the producer has got
		const Uint64 now = SDL_GetTicks();
		if ( now - title_time >= 100 )
		{
			title_time = now;
			std::ostringstream title;
			title << "Sonimation preview - " << std::fixed << std::setprecision( 2 ) << static_cast<double>( shown ) / fps << " s, frame " << shown;
			if ( store.complete() )
				title << " of " << store.size();
			else
				title << " (" << store.size() << " made so far)";
			const bool waiting = pending >= 0 || ( mode != Mode::paused && !store.complete() && cursor > last_frame() );
			if ( waiting )
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
