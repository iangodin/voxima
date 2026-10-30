#pragma once

#include "Coroutine.h"
#include "Scene.h"

#include <visuals/AABB.h>
#include <visuals/Color.h>
#include <visuals/Group.h>
#include <visuals/Point.h>
#include <visuals/Visual.h>

#include <cstddef>
#include <functional>
#include <vector>

// What an effect gets to work with: one spoken word of laid-out text, and the narration's clock.
//
// An effect is a coroutine, so it keeps its state in local variables, waits for its word, and cleans up after
// itself when it ends:
//
//     Scene my_effect( WordContext word )
//     {
//         word.word.color = dim;                                           // set up
//         co_await word.over_word( [&]( float t ) { word.word.color = ... } );   // t goes 0 to 1 as it is said
//         word.word.color = white;                                         // and tidy
//     }
//
// Run one on every word with Narration::animate_words(). The setup at the top of an effect happens as soon as
// animate_words starts, before the word is reached, so a word can be hidden (or dimmed) until then.
struct WordContext
{
	Group &text;             // the laid-out text (make_text's result); add visuals to it to draw next to the words
	Visual &word;            // this word's glyphs, at the origin of their line: change its color, position, scale
	size_t index;            // the word's number in the narration
	double start_seconds;    // when it is said, in seconds since the narration batch started
	double end_seconds;
	std::function<double()> position; // how far the narration has got now, in the same seconds
	std::function<bool()> finished;   // the narration has run out

	// The word's box in the text's coordinates (empty for a space)
	AABB bounds() const;

	// The middle of the word in its parent's coordinates: what to scale or turn it about
	Point center() const;

	// Calls fn( t ) each frame while the word is said, t going from 0 to 1 (the last call is always 1), and
	// finishes then. over_time does the same for any span of the narration.
	Scene over_word( std::function<void( float )> fn ) const;
	Scene over_time( double from, double to, std::function<void( float )> fn ) const;
};

using WordEffect = std::function<Scene( WordContext )>;

// Calls callback( t ) every frame with how far `position()` is through [from, to] (0 to 1, the last call is
// always exactly 1), waiting for `from` first, and finishes at `to` or when the narration has `finished`.
Scene track_span(
	std::function<double()> position,
	std::function<bool()> finished,
	double from,
	double to,
	std::function<void( float )> callback );

// Ready-made effects. Combine them with all(). Effects that change the same property fight; these each keep to
// their own: reveal the alpha, color the red, green and blue, pop the scale and position, underline adds a bar.
namespace word_effects
{

// Several effects on the same word at once
WordEffect all( std::vector<WordEffect> effects );

// The word is invisible until it is said, and fades in as it is
WordEffect reveal();

// The word is `before` until it is said, becomes `during` (quickly) as it is, and eases to `after` once it has been. The
// colors are blended, so give the colors it is to be seen in: for "the text color" use stage.foreground, not white (which
// would pass through a pale gray on the way, on a light background)
WordEffect color( Color before, Color during, Color after );

// The word grows to `scale` times its size and back while it is said, about its own centre
WordEffect pop( float scale = 1.15F );

// A bar draws under the word, left to right, as it is said (`gap` below the letters). It goes when the word
// is done, unless `keep`.
WordEffect underline( Color color, float thickness = 0.05F, float gap = 0.05F, bool keep = false );

}
