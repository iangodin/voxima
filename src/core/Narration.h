#pragma once

#include "Coroutine.h"
#include "Narrator.h"

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

