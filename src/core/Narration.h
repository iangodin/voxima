#pragma once

#include "Coroutine.h"
#include "Narrator.h"
#include "WordEffects.h"

#include <utils/ObserverPtr.h>

#include <visuals/Tags.h>

#include <functional>

#include <vector>

struct Stage;

class Narration
{
public:
	void setup( std::string_view name, Script &script, DubbingStage &stage );

	template<typename ...Keys>
	double speech( Keys &&...keys )
	{
		double duration = 0.0;
		for ( auto &narrator: _narrators )
		{
			auto text = _script->fetch( narrator.language(), std::forward<Keys>( keys )... ).as_multiline();
			duration = std::max( duration, narrator.speech( text ) );
		}

		return duration;
	}

	void speak( void );
	bool is_done() const;

	// Word timing of the current speech() batch, taken from the script's
	// master language (other language tracks have different timings; only one
	// can drive the animation).
	const std::vector<TimedWord> &words() const;

	// Awaitables for syncing animation to the narration. All resume once the
	// narration playback position (advanced by speak()) reaches the point;
	// they return immediately if it already has, or if nothing is being
	// narrated. Out-of-range word indices wait for the end of the narration.
	//
	//   co_await narration.until_word( 3 );      // when word 3 starts
	//   co_await narration.until_word_end( 3 );  // when word 3 finishes
	//   co_await narration.until_time( 1.5 );    // seconds since batch start
	Coroutine until_time( double seconds );
	Coroutine until_word( size_t index );
	Coroutine until_word_end( size_t index );

	// Calls callback( t ) once per frame with how far the narration has got through the span, from 0 to 1
	// (the last call is always exactly 1), then finishes. Waits for the span to start first. Use it to
	// drive something that should take as long as the narration does, such as Morph::set() (through an
	// ease if you like), so a change lands on the words being said. Times are seconds since the batch
	// started; track_words() spans from the start of word `first` to the end of word `last`.
	//
	//   co_await narration.track_words( 2, 3, [&]( float t ) { morph.set( t ); } );
	Coroutine track_time( double from, double to, std::function<void( float )> callback );
	Coroutine track_words( size_t first, size_t last, std::function<void( float )> callback );

	// Runs `effect` (see WordEffects.h and word_effects::) on every word of `text`, each starting when its
	// word does and lasting as long as the effect does. Word i of the text is word first_word + i of the
	// narration (for text shown a sentence at a time). Finishes when every effect has. Run it alongside the
	// loop that calls speak(), and do not call speech() again until it is done.
	//
	//   co_await choreograph(
	//       narrate( narration ),
	//       narration.animate_words( text, word_effects::all( { word_effects::color( grey, yellow, white ),
	//                                                            word_effects::pop() } ) ) );
	Coroutine animate_words( Group &text, WordEffect effect, size_t first_word = 0 );

	// Karaoke-style sync: as each word of the narration starts, calls
	// callback( word_visual, index ) with the matching "word<N>" group from
	// `text` (a make_styled_text()/make_text() result), and finishes after the
	// last word has started. Word N of the narration is word N of the text, so
	// the text must be the narrated line(s) as written; if the counts differ,
	// words with no counterpart on either side are skipped. Run it alongside
	// the loop that calls speak(), e.g. in a choreograph().
	//
	//   co_await narration.on_words( text, []( Visual &word, size_t ) { word.color = yellow; } );
	Coroutine on_words( Visual &text, std::function<void( Visual &, size_t )> callback );

private:
	const Narrator *master() const;

	size_t _audio_block = 0;
	ObserverPtr<Script> _script;
	std::vector<Narrator> _narrators;
};

