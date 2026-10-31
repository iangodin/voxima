// Sound: sound effects made in code (a plucked note) played as things appear, and music under them that fades in and
// out (a file, if there is one: music is not in the repository).
//
// Build it (see CMakeLists.txt), then
//
//     voxima --plugin build/sound --script-dir scripts --lang en --scene notes --preview
//     VOXIMA_MUSIC=music.mp3 voxima --plugin build/sound --script-dir scripts --lang en --scene notes --preview
//
// With --preview, rebuilding it while the window is open plays the scene again with the change.

#include <core/Animate.h>
#include <core/Plugin.h>
#include <core/SceneRegistry.h>
#include <core/Script.h>
#include <core/Stage.h>
#include <core/Wait.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

using namespace std::chrono_literals;

VOXIMA_PLUGIN();

namespace
{
// --8<-- [start:pluck]
// A plucked note, made rather than read from a file: a sine that dies away over `seconds`
AudioBuffer pluck( float frequency, float seconds = 2.F )
{
	constexpr size_t rate = 48000;
	AudioBuffer note( static_cast<size_t>( seconds * rate ), rate, 1 );
	for ( size_t f = 0; f < note.frames(); ++f )
	{
		const float t = static_cast<float>( f ) / rate;
		const float attack = std::min( 1.F, t / 0.005F ); // no click at the start
		note.frame( f )[0] = attack * std::exp( -3.F * t / seconds * 2.F ) * std::sin( 2.F * 3.14159265F * frequency * t );
	}
	return note;
}
// --8<-- [end:pluck]

// Three circles, each arriving with a note of a chord, over music (if VOXIMA_MUSIC names a file)
// --8<-- [start:scene]
Scene notes( Stage &stage, Script & )
{
	// --8<-- [start:music]
	Sound music;
	if ( const char *file = std::getenv( "VOXIMA_MUSIC" ); file != nullptr && std::filesystem::exists( file ) )
	{
		music = stage.audio.play( file, { .volume = -22.0, .loop = true, .fade_in = 2s } );
	}
	else
	{
		std::cerr << "No music: set VOXIMA_MUSIC to a sound file to have some\n";
	}
	// --8<-- [end:music]

	const Palette &palette = stage.theme->palette();
	const Color colors[] = { palette.blue, palette.green, palette.red };
	const float notes[] = { 261.63F, 329.63F, 392.F }; // C, E and G

	for ( int i = 0; i < 3; ++i )
	{
		auto circle = std::make_unique<Circle>();
		circle->radius = 0.9F;
		circle->color = colors[i];
		auto &dot = stage.visuals.add( std::move( circle ) );
		dot.position = { -3.F + 3.F * static_cast<float>( i ), 0.F };

		// --8<-- [start:tie]
		// The sound starts at the frame being made, so with the animation
		stage.audio.play( pluck( notes[i] ), { .volume = -14.0 } );
		co_await grow_in( stage, dot, 0.5s );
		// --8<-- [end:tie]
		co_await wait_for( stage, 0.4s );
	}

	co_await wait_for( stage, 2s );
	// --8<-- [start:fade-out]
	music.fade_out( 2s );
	// --8<-- [end:fade-out]
	co_await wait_for( stage, 2s );
}
// --8<-- [end:scene]
}

REGISTER_SCENE( "notes", "Three circles, each with a note of a chord, over music if there is a file", notes );
