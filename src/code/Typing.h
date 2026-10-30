#pragma once

#include "CreateCode.h"

#include <core/Scene.h>

#include <chrono>
#include <cstdint>
#include <string_view>
#include <vector>

struct Stage;

// How code is typed
struct TypingRhythm
{
	// Characters a second, on average, pauses and all (so N characters take about N / speed seconds). Quicker than a
	// person types, but it still reads as typing; above about 60 (at 24 fps; 150 at 60 fps) even the pauses at the
	// ends of lines are shorter than a frame, and it becomes a wipe.
	double chars_per_second = 36.0;

	// Unevenly, like a person: quicker within a word, slower after a space or at the end of a statement, a
	// clear pause at the end of each line (and now and then a longer one, thinking), and a little variation on every
	// key. Otherwise every character takes the same time.
	bool human = true;

	// Which uneven rhythm (the same seed always gives the same one, on any machine: a render is repeatable)
	uint64_t seed = 1;

	// How long a character takes to fade in once it is typed
	double fade_seconds = 0.05;
};

// When each character of `text` is typed, in seconds from the first (which is typed at 0): one time per byte of
// the text (the bytes of a character share its time). Leading indentation comes at once with the line, as an
// editor indents. The last character's time is about (characters - 1) / chars_per_second.
std::vector<double> typing_schedule( std::string_view text, const TypingRhythm &rhythm = {} );

// Types a listing (make_code) a character at a time: each appears when its time comes (see typing_schedule), fading
// in. Everything starts hidden. Like a Morph, it is set to a point, so anything can drive it: its own type(), a
// tween, or the narration:
//
//     auto &listing = stage.visuals.add( make_code( *stage.theme, code::Language::CPP, source ) );
//     Typewriter typer( listing );
//     co_await typer.type( stage );          // at the rhythm's own speed
//     co_await typer.type( stage, 4s );      // or fitted to 4 seconds, the rhythm kept
//     co_await narration.track_markers( "a", "b", [&]( float t ) { typer.set( t ); } );
//
// The listing must stay alive (on the stage) while the Typewriter is used.
class Typewriter
{
public:
	explicit Typewriter( CodeListing &listing, const TypingRhythm &rhythm = {} );

	// How long the typing takes, to the last character fully faded in
	double duration() const
	{
		return _duration;
	}

	// Show what has been typed `seconds` into the typing (0: nothing yet; duration(): all of it)
	void set_time( double seconds );

	// The same as a fraction of the whole, 0 to 1
	void set( float t )
	{
		set_time( static_cast<double>( t ) * _duration );
	}

	// Type it all, at the rhythm's speed or in the time given
	Scene type( const Stage &stage );
	Scene type( const Stage &stage, std::chrono::duration<double> duration );

private:
	struct Key
	{
		Glyph *glyph;
		double time;
		float alpha; // the glyph's own alpha, which it fades in to
	};

	std::vector<Key> _keys;
	double _fade;
	double _duration = 0.0;
};
